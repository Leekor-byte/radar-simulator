#pragma once
#include <QString>

class RadarCalculator {
public:
    // Уравнение радиолокации (принимаемая мощность)
    static double calculateReceivedPower(double Pt, double Gt, double Gr,
                                         double lambda, double sigma, double R);

    // Временной анализ
    struct TimeAnalysisResult {
        bool feasible;
        double maxRange;      // метры
        double pulsePeriod;   // секунды
        QString message;
    };
    static TimeAnalysisResult performTimeAnalysis(double rpm, int azimuthPoints,
                                                  double tau, double beamwidth,
                                                  const QString& signalType);

    // Мощностной анализ
    struct PowerAnalysisResult {
        double maxRange;      // метры
    };
    static PowerAnalysisResult performPowerAnalysis(double tau, double period,
                                                    double Pt, double G,
                                                    double lambda,
                                                    double gainRx_dB,
                                                    const QString &signalType);

    // Комбинированный анализ
    struct CombinedAnalysisResult {
        double timeRange;
        double powerRange;
        int pulseCount;
        double receivedPower;      // если ограничение по времени
        double recommendedPt;      // если ограничение по мощности
        bool isTimeLimited;
        QString conclusion;
    };
    static CombinedAnalysisResult performCombinedAnalysis(
        double rpm, int azimuthPoints, double tau, double period,
        double beamwidth, double Pt, double G, double lambda,
        double gainRx_dB, const QString &signalType);
};
