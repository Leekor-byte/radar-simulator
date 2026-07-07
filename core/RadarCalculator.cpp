#include "RadarCalculator.h"
#include <cmath>
#include <QString>

// Константы
static const double C = 3.0e8;
static const double PI = 3.141592653589793;
static const double BOLTZMANN = 1.38e-23;
static const double T0 = 290.0;

// Вспомогательная функция: проверка попадания в диапазон
static bool inRange(double val, double min, double max) {
    return val >= min && val <= max;
}

// Принимаемая мощность по уравнению радиолокации
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

    // Проверка диапазонов: rpm
    if (!inRange(rpm, 10.0, 40.0)) {
        res.message = "Rotation speed out of range [10, 40] rpm";
        return res;
    }
    if (!inRange(azimuthPoints, 1024, 16384)) {
        res.message = "Azimuth points out of range [1024, 16384]";
        return res;
    }

    double tau_us = tau; // предположим, что tau уже в микросекундах, в UI будем переводить
    double tau_sec = tau_us * 1e-6;
    double rpm_sec = rpm / 60.0; // об/сек

    // Период повторения = время на одну азимутальную точку
    double T = 1.0 / (rpm_sec * azimuthPoints);  // сек

    // Проверка для типа сигнала
    bool tauOk = false, periodOk = false;
    if (signalType == "Impulse") {
        tauOk = inRange(tau_us, 0.05, 1.0);
        periodOk = inRange(T * 1e6, 340.0, 2000.0);
    } else { // Quasi-continuous
        tauOk = inRange(tau_us, 0.05, 30.0);
        periodOk = inRange(T * 1e6, 10.0, 200.0);
    }
    if (!tauOk || !periodOk) {
        res.message = "Tau or period out of allowed range for selected signal type";
        return res;
    }

    if (T <= tau_sec) {
        res.message = "Pulse period must be greater than pulse width";
        return res;
    }

    // Время облучения цели лучом
    double T_illum = beamwidth / (6.0 * rpm);  // сек
    if (T_illum < T) {
        res.message = "Illumination time is less than pulse period";
        return res;
    }

    // Всё проверено, считаем дальность
    res.feasible = true;
    res.pulsePeriod = T;
    res.maxRange = C * (T - tau_sec) / 2.0;
    res.message = "Feasible";
    return res;
}

RadarCalculator::PowerAnalysisResult RadarCalculator::performPowerAnalysis(
    double tau, double period, double Pt, double G, double lambda,
    double gainRx_dB, const QString &signalType)
{
    PowerAnalysisResult res;
    double tau_sec = tau * 1e-6;
    double T_sec = period * 1e-6;

    // Проверка диапазона мощности
    bool powerOk = false;
    if (signalType == "Impulse") {
        powerOk = inRange(Pt, 5000.0, 60000.0);
    } else {
        powerOk = inRange(Pt, 1.0, 200.0);
    }
    if (!powerOk) {
        res.maxRange = 0.0; // некорректно, можно выбросить
        return res;
    }

    // Пороговая чувствительность (упрощённо: S_min = k*T0*B / G_rx)
    double B = 1.0 / tau_sec;
    double G_rx_lin = pow(10.0, gainRx_dB / 10.0);
    double S_min = (BOLTZMANN * T0 * B) / G_rx_lin;

    // Энергия импульса
    double E = 0.0;
    if (signalType == "Impulse") {
        E = Pt * tau_sec;               // пиковая * длительность
    } else {
        E = Pt * T_sec;                 // средняя * период
    }

    double sigma = 10.0;  // ЭПР = 10 м²
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
    // 1. Временной анализ
    TimeAnalysisResult timeRes = performTimeAnalysis(rpm, azimuthPoints, tau, beamwidth, signalType);
    double R_time = timeRes.feasible ? timeRes.maxRange : 0.0;
    double T = timeRes.pulsePeriod; // берём период из временного анализа (или из входного?)
    // Если используем введённый период, а не вычисленный, то надо аккуратно.
    // По заданию на входе комбинированного задаются все параметры, включая период.
    // Используем введённый период для расчёта дальности (временная дальность зависит от периода).
    // Так что R_time должна вычисляться на основе введённого периода, а не вычисленного T.
    // Поэтому переопределим R_time по формуле с пользовательским периодом.
    double tau_sec = tau * 1e-6;
    double T_sec = period * 1e-6;
    R_time = (T_sec > tau_sec) ? (C * (T_sec - tau_sec) / 2.0) : 0.0;
    res.timeRange = R_time;

    // 2. Мощностной анализ
    PowerAnalysisResult powRes = performPowerAnalysis(tau, period, Pt, G, lambda, gainRx_dB, signalType);
    res.powerRange = powRes.maxRange;

    // 3. Количество импульсов в пачке
    double T_illum = beamwidth / (6.0 * rpm);
    double N_pulses = T_illum / T_sec;
    res.pulseCount = static_cast<int>(floor(N_pulses)); // целое количество

    // 4. Сравнение
    if (R_time < powRes.maxRange) {
        res.isTimeLimited = true;
        res.receivedPower = calculateReceivedPower(Pt, G, G, lambda, 10.0, R_time);
        res.recommendedPt = 0.0;
        res.conclusion = QString("Time limited. Received power at R_time: %1 W").arg(res.receivedPower);
    } else {
        res.isTimeLimited = false;
        // Рекомендуемая мощность передатчика, чтобы R_power = R_time
        double B = 1.0 / tau_sec;
        double G_rx_lin = pow(10.0, gainRx_dB / 10.0);
        double S_min = (BOLTZMANN * T0 * B) / G_rx_lin;
        double sigma = 10.0;
        double E_new = pow(R_time, 4) * pow(4 * PI, 3) * S_min / (G * G * lambda * lambda * sigma);
        if (signalType == "Impulse") {
            res.recommendedPt = E_new / tau_sec;
        } else {
            res.recommendedPt = E_new / T_sec;
        }
        res.receivedPower = 0.0;
        res.conclusion = QString("Power limited. Recommended Tx power: %1 W").arg(res.recommendedPt);
    }
    return res;
}
