#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifndef EIGEN_DEFAULT_TO_ROW_MAJOR
#define EIGEN_DEFAULT_TO_ROW_MAJOR
#endif

#ifndef EIGEN_INITIALIZE_MATRICES_BY_ZERO
#define EIGEN_INITIALIZE_MATRICES_BY_ZERO
#endif

#include <QMainWindow>
#include <complex>
#include <QtWidgets/QLabel>
#include <QMessageBox>

#include "fftw3.h"
#include "tiffio.h"

#include <Eigen/Dense>
#include "gpa.h"
#include <iostream>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_actionOpen_triggered();

    void clickGVector(QMouseEvent *event);

    void clickBraggSpot(QMouseEvent *event);

    void processBraggClick(double x, double y);

    void clickRectCorner(QMouseEvent *event);

    void on_actionGPA_triggered();

    void on_actionHann_triggered();

    void on_actionMinimal_dialogs_triggered();

    void on_actionReuse_gs_triggered();

    void on_leftCombo_currentIndexChanged(int index);

    void on_rightCombo_currentIndexChanged(int index);

    void on_limitsSpin_editingFinished();

    void on_colourMapBox_currentIndexChanged(const QString &Map);

    void ExportAll(int choice);

    void ExportStrains(int choice);

    void on_actionExportAllIm_triggered() {ExportAll(0);}
    void on_actionExportAllDat_triggered() {ExportAll(1);}
    void on_actionExportAllBin_triggered() {ExportAll(2);}

    void on_actionExportStrainsIm_triggered() {ExportStrains(0);}
    void on_actionExportStrainsDat_triggered() {ExportStrains(1);}
    void on_actionExportStrainsBin_triggered() {ExportStrains(2);}
    void on_angleSpin_editingFinished();

    void on_resultModeBox_currentIndexChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;

    bool minimalDialogs, reuseGs;

    Eigen::MatrixXcd original_image;

    std::unique_ptr<GPA> GPAstrain;

    QString dialogPath;

    std::vector<double> xCorner, yCorner;

    QLabel *statusLabel;

    int phaseSelection;

    double lastAngle;
    //
    std::vector<double> lastG1, lastG2;

    double minGrad, _sig;

    bool haveImage = false;
    bool haveStrains = false;

    void updateStatusBar(QString message);

    void openTIFF(std::string filename);

    void DisconnectAll();

    void ClearImages();

    template <typename T>
    void readTiffData(TIFF* tif)
    {
        uint32 imagelength;
        tsize_t scanline;
        tdata_t buf;

        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imagelength);
        scanline = TIFFScanlineSize(tif);
        buf = _TIFFmalloc(scanline);
        Eigen::MatrixXcd complexImage(imagelength, scanline/sizeof(T));

        // image too small to differentiate
        if (imagelength < 3 || scanline/sizeof(T) < 3)
        {
            QMessageBox::information(this, tr("File open"), tr("Image too small."), QMessageBox::Ok);
            return;
        }

        for (uint32 row = 0; row < imagelength; ++row)
        {
            TIFFReadScanline(tif, buf, row);
            for (uint32 col = 0; col < scanline; ++col) // remember this is in bytes
            {
                T* data = (T*)buf;
                complexImage(row, col/sizeof(T)) = static_cast<double>(data[col/sizeof(T)]);
            }
        }

        _TIFFfree(buf);
        original_image = complexImage.colwise().reverse();
        showImageAndFFT(original_image);
    }

    void openDM(std::string filename);

    void showImageAndFFT(Eigen::MatrixXcd &image);

    void selectRefineArea();

    void doRefinement(double top, double left, double bottom, double right);

    void continuePhase();

    void getStrains();

    void updateOtherPlot(int index, int side, bool rePlot = true);

    void AcceptGVector();

};

#endif // MAINWINDOW_H
