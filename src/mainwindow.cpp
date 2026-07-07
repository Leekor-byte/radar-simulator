#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "RadarCalculator.h"
#include <QString>
#include <QDoubleValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Установка валидаторов и подсказок
    ui->leFrequency->setPlaceholderText("9400 - 9800 MHz");
    ui->leFrequency->setValidator(new QDoubleValidator(9400, 9800, 2, this));
    ui->leRotationSpeed->setPlaceholderText("10 - 40 rpm");
    ui->leAzimuthPoints->setPlaceholderText("1024 - 16384");
    ui->leAzimuthPoints->setValidator(new QIntValidator(1024, 16384, this));

    ui->leGainAntenna->setPlaceholderText("100 - 500 (times)");
    ui->leGainRx->setPlaceholderText("5 - 50 dB");
    ui->leBeamwidth->setPlaceholderText("deg");

    // Поля для временного/мощностного анализа
    ui->leTau->setPlaceholderText("pulse width (µs)");
    ui->lePeriod->setPlaceholderText("pulse period (µs)");
    ui->lePt->setPlaceholderText("Tx power (W)");

    connect(ui->btnCalculate, &QPushButton::clicked,
            this, &MainWindow::onCalculateClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::readCommonParams(double &freq, double &lambda,
                                  double &G, double &gainRx_dB,
                                  QString &antennaType, double &beamwidth)
{
    bool ok;
    freq = ui->leFrequency->text().toDouble(&ok);
    if (!ok) return false;
    // Перевод частоты в длину волны
    const double c = 3e8;
    lambda = c / (freq * 1e6);

    G = ui->leGainAntenna->text().toDouble(&ok);
    if (!ok) return false;

    gainRx_dB = ui->leGainRx->text().toDouble(&ok);
    if (!ok) return false;

    antennaType = ui->antennaBox->currentText();
    beamwidth = ui->leBeamwidth->text().toDouble(&ok);
    if (!ok) return false;

    // Проверка диапазонов для общих параметров
    if (freq < 9400 || freq > 9800) return false;
    if (G < 100 || G > 500) return false;
    if (gainRx_dB < 5 || gainRx_dB > 50) return false;
    if (antennaType == "Slot") {
        if (beamwidth < 0.2 || beamwidth > 4.0) return false;
    } else { // Mirror
        if (beamwidth < 3.0 || beamwidth > 6.0) return false;
    }
    return true;
}

void MainWindow::showResult(const QString &text) {
    ui->labelResult->setText(text);
}

void MainWindow::onCalculateClicked()
{
    QString mode = ui->modeBox->currentText();
    QString signalType = ui->signalModeBox->currentText();

    // Чтение общих параметров
    double freq, lambda, G, gainRx_dB, beamwidth;
    QString antennaType;
    if (!readCommonParams(freq, lambda, G, gainRx_dB, antennaType, beamwidth)) {
        showResult("Error: invalid common parameters");
        return;
    }

    // Чтение параметров, зависящих от режима
    double rpm = ui->leRotationSpeed->text().toDouble();
    int azimuthPoints = ui->leAzimuthPoints->text().toInt();
    double tau = ui->leTau->text().toDouble();   // мкс
    double period = ui->lePeriod->text().toDouble(); // мкс
    double Pt = ui->lePt->text().toDouble();       // Вт

    QString result;

    if (mode == "Time Analysis") {
        // Проверка наличия rpm, azimuthPoints, tau
        if (rpm <= 0 || azimuthPoints <= 0 || tau <= 0) {
            showResult("Error: invalid input for time analysis");
            return;
        }
        auto res = RadarCalculator::performTimeAnalysis(rpm, azimuthPoints, tau, beamwidth, signalType);
        if (res.feasible) {
            result = QString("Time Analysis: FEASIBLE\n"
                             "Max range: %1 m\n"
                             "Pulse period: %2 µs")
                         .arg(res.maxRange)
                         .arg(res.pulsePeriod * 1e6);
        } else {
            result = "Time Analysis: NOT FEASIBLE\n" + res.message;
        }
    }
    else if (mode == "Power Analysis") {
        if (tau <= 0 || period <= 0 || Pt <= 0) {
            showResult("Error: invalid input for power analysis");
            return;
        }
        auto res = RadarCalculator::performPowerAnalysis(tau, period, Pt, G, lambda, gainRx_dB, signalType);
        if (res.maxRange > 0) {
            result = QString("Power Analysis:\n"
                             "Max range for RCS 10 m²: %1 m").arg(res.maxRange);
        } else {
            result = "Power Analysis: invalid parameters";
        }
    }
    else if (mode == "Combined Analysis") {
        if (rpm <= 0 || azimuthPoints <= 0 || tau <= 0 || period <= 0 || Pt <= 0) {
            showResult("Error: invalid input for combined analysis");
            return;
        }
        auto res = RadarCalculator::performCombinedAnalysis(rpm, azimuthPoints, tau, period,
                                                            beamwidth, Pt, G, lambda, gainRx_dB, signalType);
        result = QString("Combined Analysis:\n"
                         "Time range: %1 m\n"
                         "Power range: %2 m\n"
                         "Pulse count: %3\n\n"
                         "%4")
                     .arg(res.timeRange)
                     .arg(res.powerRange)
                     .arg(res.pulseCount)
                     .arg(res.conclusion);
    }
    showResult(result);
}
