#include "RadarCalculator.h"
#include <cmath>

static const double C = 3.0e8;
static const double PI = 3.141592653589793;
static const double BOLTZMANN = 1.38e-23;
static const double T0 = 290.0;

static bool inRange(double val, double min, double max) {
    return val >= min && val <= max;
}

double RadarCalculator::calculateReceivedPower(double Pt, double Gt, double Gr,
                                               double lambda, double sigma, double R)
{
    double num = Pt * Gt * Gr * lambda * lambda * sigma;
    double den = pow(4 * PI, 3) * pow(R, 4);
    return num / den;
}

RadarCalculator::TimeAnalysisResult RadarCalculator::performTimeAnalysis(
    double rpm, int azimuthPoints, double tau, double beamwidth,
    const QString &signalType)
{
    TimeAnalysisResult res;
    res.feasible = false;
    res.maxRange = 0.0;
    res.pulsePeriod = 0.0;

    if (!inRange(rpm, 10.0, 40.0)) {
        res.message = "Скорость вращения вне диапазона [10, 40] об/мин";
        return res;
    }
    if (!inRange(azimuthPoints, 1024, 16384)) {
        res.message = "Количество точек по азимуту вне диапазона [1024, 16384]";
        return res;
    }

    double tau_us = tau;
    double tau_sec = tau_us * 1e-6;
    double rpm_sec = rpm / 60.0;

    double T = 1.0 / (rpm_sec * azimuthPoints);
    double T_us = T * 1e6;   // период в микросекундах для сообщения

    bool tauOk = false, periodOk = false;
    if (signalType == "Импульсный") {
        tauOk = inRange(tau_us, 0.05, 1.0);
        periodOk = inRange(T_us, 340.0, 2000.0);
        if (!tauOk && !periodOk)
            res.message = QString("Длительность %1 мкс вне [0.05–1] мкс и период %2 мкс вне [340–2000] мкс")
                              .arg(tau_us).arg(T_us);
        else if (!tauOk)
            res.message = QString("Длительность %1 мкс вне допустимого диапазона [0.05–1] мкс").arg(tau_us);
        else if (!periodOk)
            res.message = QString("Период %1 мкс вне допустимого диапазона [340–2000] мкс (вычислен из скорости и точек)").arg(T_us);
    } else { // Квазинепрерывный
        tauOk = inRange(tau_us, 0.05, 30.0);
        periodOk = inRange(T_us, 10.0, 200.0);
        if (!tauOk && !periodOk)
            res.message = QString("Длительность %1 мкс вне [0.05–30] мкс и период %2 мкс вне [10–200] мкс")
                              .arg(tau_us).arg(T_us);
        else if (!tauOk)
            res.message = QString("Длительность %1 мкс вне допустимого диапазона [0.05–30] мкс").arg(tau_us);
        else if (!periodOk)
            res.message = QString("Период %1 мкс вне допустимого диапазона [10–200] мкс (вычислен из скорости и точек)").arg(T_us);
    }
    if (!tauOk || !periodOk) {
        return res;
    }

    if (T <= tau_sec) {
        res.message = "Период повторения должен быть больше длительности импульса";
        return res;
    }

    double T_illum = beamwidth / (6.0 * rpm);
    if (T_illum < T) {
        res.message = "Время облучения меньше периода повторения";
        return res;
    }

    res.feasible = true;
    res.pulsePeriod = T;
    res.maxRange = C * (T - tau_sec) / 2.0;
    res.message = "Режим возможен";
    return res;
}

RadarCalculator::PowerAnalysisResult RadarCalculator::performPowerAnalysis(
    double tau, double period, double Pt, double G, double lambda,
    double gainRx_dB, const QString &signalType)
{
    PowerAnalysisResult res;
    double tau_sec = tau * 1e-6;
    double T_sec = period * 1e-6;

    bool powerOk = false;
    if (signalType == "Импульсный") {
        powerOk = inRange(Pt, 5000.0, 60000.0);
    } else {
        powerOk = inRange(Pt, 1.0, 200.0);
    }
    if (!powerOk) {
        res.maxRange = 0.0;
        return res;
    }

    double B = 1.0 / tau_sec;
    double G_rx_lin = pow(10.0, gainRx_dB / 10.0);
    double S_min = (BOLTZMANN * T0 * B) / G_rx_lin;

    double E = 0.0;
    if (signalType == "Импульсный") {
        E = Pt * tau_sec;
    } else {
        E = Pt * T_sec;
    }

    double sigma = 10.0;
    double Gt = G;
    double Gr = G;

    double numerator = E * Gt * Gr * lambda * lambda * sigma;
    double denominator = pow(4 * PI, 3) * S_min;
    res.maxRange = pow(numerator / denominator, 0.25);
    return res;
}

RadarCalculator::CombinedAnalysisResult RadarCalculator::performCombinedAnalysis(
    double rpm, int azimuthPoints, double tau, double period,
    double beamwidth, double Pt, double G, double lambda,
    double gainRx_dB, const QString &signalType)
{
    CombinedAnalysisResult res;
    TimeAnalysisResult timeRes = performTimeAnalysis(rpm, azimuthPoints, tau, beamwidth, signalType);
    double R_time = timeRes.feasible ? timeRes.maxRange : 0.0;
    double tau_sec = tau * 1e-6;
    double T_sec = period * 1e-6;
    R_time = (T_sec > tau_sec) ? (C * (T_sec - tau_sec) / 2.0) : 0.0;
    res.timeRange = R_time;

    PowerAnalysisResult powRes = performPowerAnalysis(tau, period, Pt, G, lambda, gainRx_dB, signalType);
    res.powerRange = powRes.maxRange;

    double T_illum = beamwidth / (6.0 * rpm);
    double N_pulses = T_illum / T_sec;
    res.pulseCount = static_cast<int>(floor(N_pulses));

    if (R_time < powRes.maxRange) {
        res.isTimeLimited = true;
        res.receivedPower = calculateReceivedPower(Pt, G, G, lambda, 10.0, R_time);
        res.recommendedPt = 0.0;
        res.conclusion = QString("Ограничение по времени. Принимаемая мощность на дальности временного ограничения: %1 Вт")
                             .arg(res.receivedPower);
    } else {
        res.isTimeLimited = false;
        double B = 1.0 / tau_sec;
        double G_rx_lin = pow(10.0, gainRx_dB / 10.0);
        double S_min = (BOLTZMANN * T0 * B) / G_rx_lin;
        double sigma = 10.0;
        double E_new = pow(R_time, 4) * pow(4 * PI, 3) * S_min / (G * G * lambda * lambda * sigma);
        if (signalType == "Импульсный") {
            res.recommendedPt = E_new / tau_sec;
        } else {
            res.recommendedPt = E_new / T_sec;
        }
        res.receivedPower = 0.0;
        res.conclusion = QString("Ограничение по мощности. Рекомендуемая мощность передатчика: %1 Вт")
                             .arg(res.recommendedPt);
    }
    return res;
}
