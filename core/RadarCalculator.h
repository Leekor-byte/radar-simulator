#pragma once

#include <QString>

class RadarCalculator {
public:
    static double calculateReceivedPower(double Pt, double Gt, double Gr,
                                         double lambda, double sigma, double R);

    struct TimeAnalysisResult {
        bool feasible;
        double maxRange;
        double pulsePeriod;   // секунды
        QString message;
    };
    static TimeAnalysisResult performTimeAnalysis(double rpm, int azimuthPoints,
                                                  double tau, double beamwidth,
                                                  const QString &signalType);

    struct PowerAnalysisResult {
        double maxRange;
    };
    static PowerAnalysisResult performPowerAnalysis(double tau, double period,
                                                    double Pt, double G,
                                                    double lambda,
                                                    double gainRx_dB,
                                                    const QString &signalType);

    struct CombinedAnalysisResult {
        double timeRange;
        double powerRange;
        int pulseCount;
        double receivedPower;
        double recommendedPt;
        bool isTimeLimited;
        QString conclusion;
    };
    static CombinedAnalysisResult performCombinedAnalysis(
        double rpm, int azimuthPoints, double tau, double period,
        double beamwidth, double Pt, double G, double lambda,
        double gainRx_dB, const QString &signalType);
};
