#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "RadarCalculator.h"
#include <QDoubleValidator>
#include <QIntValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Валидаторы
    ui->leFrequency->setValidator(new QDoubleValidator(9400, 9800, 2, this));
    ui->leAzimuthPoints->setValidator(new QIntValidator(1024, 16384, this));

    // Начальные подсказки (некоторые статичны, остальные динамические)
    ui->leFrequency->setPlaceholderText("9400 – 9800 МГц");
    ui->leGainAntenna->setPlaceholderText("100 – 500 раз");
    ui->leGainRx->setPlaceholderText("5 – 50 дБ");
    // leBeamwidth – динамический placeholder
    ui->leRotationSpeed->setPlaceholderText("10 – 40 об/мин");
    ui->leAzimuthPoints->setPlaceholderText("1024 – 16384");
    // leTau, lePeriod, lePt – динамические

    connect(ui->btnCalculate, &QPushButton::clicked,
            this, &MainWindow::onCalculateClicked);
    connect(ui->modeBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModeChanged);
    connect(ui->signalModeBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSignalTypeChanged);
    connect(ui->antennaBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAntennaTypeChanged);

    // Установка начального состояния
    onModeChanged(ui->modeBox->currentIndex());
    onSignalTypeChanged(ui->signalModeBox->currentIndex());
    onAntennaTypeChanged(ui->antennaBox->currentIndex());
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
    const double c = 3e8;
    lambda = c / (freq * 1e6);

    G = ui->leGainAntenna->text().toDouble(&ok);
    if (!ok) return false;

    gainRx_dB = ui->leGainRx->text().toDouble(&ok);
    if (!ok) return false;

    antennaType = ui->antennaBox->currentText();
    beamwidth = ui->leBeamwidth->text().toDouble(&ok);
    if (!ok) return false;

    if (freq < 9400 || freq > 9800) return false;
    if (G < 100 || G > 500) return false;
    if (gainRx_dB < 5 || gainRx_dB > 50) return false;
    if (antennaType == "Щелевая") {
        if (beamwidth < 0.2 || beamwidth > 4.0) return false;
    } else {
        if (beamwidth < 3.0 || beamwidth > 6.0) return false;
    }
    return true;
}

void MainWindow::showResult(const QString &text) {
    ui->labelResult->setText(text);
}

void MainWindow::onModeChanged(int index) {
    QString mode = ui->modeBox->currentText();
    bool isTime = (mode == "Временной анализ");
    bool isPower = (mode == "Мощностной анализ");
    bool isCombined = (mode == "Комбинированный анализ");

    // Всегда видимые контейнеры
    ui->containerFreq->setVisible(true);
    ui->containerGainAntenna->setVisible(true);
    ui->containerGainRx->setVisible(true);
    ui->containerBeamwidth->setVisible(true);
    ui->containerTau->setVisible(true);

    // Мощность и период — в мощностном и комбинированном
    bool showPower = isPower || isCombined;
    ui->containerPower->setVisible(showPower);
    ui->containerPeriod->setVisible(showPower);

    // Скорость вращения и точки — во временном и комбинированном
    bool showTime = isTime || isCombined;
    ui->containerRPM->setVisible(showTime);
    ui->containerAzimuth->setVisible(showTime);
}

void MainWindow::onSignalTypeChanged(int index) {
    QString signalType = ui->signalModeBox->currentText();
    if (signalType == "Импульсный") {
        ui->leTau->setPlaceholderText("0.05 – 1 мкс");
        ui->lePeriod->setPlaceholderText("340 – 2000 мкс");
        ui->lePt->setPlaceholderText("5000 – 60000 Вт");
    } else {
        ui->leTau->setPlaceholderText("0.05 – 30 мкс");
        ui->lePeriod->setPlaceholderText("10 – 200 мкс");
        ui->lePt->setPlaceholderText("1 – 200 Вт");
    }
}

void MainWindow::onAntennaTypeChanged(int index) {
    QString antenna = ui->antennaBox->currentText();
    if (antenna == "Щелевая") {
        ui->leBeamwidth->setPlaceholderText("0.2 – 4°");
    } else {
        ui->leBeamwidth->setPlaceholderText("3 – 6°");
    }
}

void MainWindow::onCalculateClicked()
{
    QString mode = ui->modeBox->currentText();
    QString signalType = ui->signalModeBox->currentText();

    double freq, lambda, G, gainRx_dB, beamwidth;
    QString antennaType;
    if (!readCommonParams(freq, lambda, G, gainRx_dB, antennaType, beamwidth)) {
        showResult("Ошибка: неверные общие параметры");
        return;
    }

    double rpm = ui->leRotationSpeed->text().toDouble();
    int azimuthPoints = ui->leAzimuthPoints->text().toInt();
    double tau = ui->leTau->text().toDouble();
    double period = ui->lePeriod->text().toDouble();
    double Pt = ui->lePt->text().toDouble();

    QString result;

    if (mode == "Временной анализ") {
        if (rpm <= 0 || azimuthPoints <= 0 || tau <= 0) {
            showResult("Ошибка: неверные входные данные для временного анализа");
            return;
        }
        auto res = RadarCalculator::performTimeAnalysis(rpm, azimuthPoints, tau, beamwidth, signalType);
        if (res.feasible) {
            result = QString("Временной анализ: РЕЖИМ ВОЗМОЖЕН\n"
                             "Максимальная дальность: %1 м\n"
                             "Период импульсов: %2 мкс")
                         .arg(res.maxRange)
                         .arg(res.pulsePeriod * 1e6);
        } else {
            result = "Временной анализ: НЕВОЗМОЖЕН\n" + res.message;
        }
    }
    else if (mode == "Мощностной анализ") {
        if (tau <= 0 || period <= 0 || Pt <= 0) {
            showResult("Ошибка: неверные входные данные для мощностного анализа");
            return;
        }
        auto res = RadarCalculator::performPowerAnalysis(tau, period, Pt, G, lambda, gainRx_dB, signalType);
        if (res.maxRange > 0) {
            result = QString("Мощностной анализ:\n"
                             "Максимальная дальность для цели с ЭПР 10 м²: %1 м").arg(res.maxRange);
        } else {
            result = "Мощностной анализ: неверные параметры";
        }
    }
    else if (mode == "Комбинированный анализ") {
        if (rpm <= 0 || azimuthPoints <= 0 || tau <= 0 || period <= 0 || Pt <= 0) {
            showResult("Ошибка: неверные входные данные для комбинированного анализа");
            return;
        }
        auto res = RadarCalculator::performCombinedAnalysis(rpm, azimuthPoints, tau, period,
                                                            beamwidth, Pt, G, lambda, gainRx_dB, signalType);
        result = QString("Комбинированный анализ:\n"
                         "Дальность по времени: %1 м\n"
                         "Дальность по мощности: %2 м\n"
                         "Количество импульсов в пачке: %3\n\n"
                         "%4")
                     .arg(res.timeRange)
                     .arg(res.powerRange)
                     .arg(res.pulseCount)
                     .arg(res.conclusion);
    }
    showResult(result);
}
