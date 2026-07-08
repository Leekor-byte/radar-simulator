#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCalculateClicked();
    void onModeChanged(int index);
    void onSignalTypeChanged(int index);
    void onAntennaTypeChanged(int index);

private:
    Ui::MainWindow *ui;

    bool readCommonParams(double &freq, double &lambda,
                          double &G, double &gainRx_dB,
                          QString &antennaType, double &beamwidth);
    void showResult(const QString &text);
};

#endif // MAINWINDOW_H
