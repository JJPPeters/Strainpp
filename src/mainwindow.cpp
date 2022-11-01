#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtSvg/QSvgRenderer>
#include "dmreader.h"
#include "utils.h"
#include "versiondialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    lastG1 = std::vector<double>();
    lastG2 = std::vector<double>();

    QCoreApplication::setOrganizationName("PetersSoft");
    QCoreApplication::setApplicationName("Strain++");

    QSettings settings;
    if (!settings.contains("dialog/currentPath"))
        settings.setValue("dialog/currentPath", QStandardPaths::HomeLocation);
    if (!settings.contains("dialog/currentSavePath"))
        settings.setValue("dialog/currentSavePath", QStandardPaths::HomeLocation);
    minimalDialogs = settings.contains("dialog/minimal") && settings.value("dialog/minimal").toBool();
    reuseGs = settings.contains("dialog/reuseGs") && settings.value("dialog/reuseGs").toBool();

    ui->setupUi(this);

    setWindowTitle("Strain++");

    ui->actionMinimal_dialogs->setChecked(minimalDialogs);
    ui->actionReuse_gs->setChecked(reuseGs);

    // add label to  status bar (can't be done with designer)
    statusLabel = new QLabel("--");
    statusLabel->setContentsMargins(10,0,0,0);
    statusLabel->setSizePolicy(QSizePolicy::MinimumExpanding,
                               QSizePolicy::MinimumExpanding);

    ui->colorBar->SetColorMap(ui->colourMapBox->currentText(), true);
    ui->colorBar->SetLimits(ui->limitsSpin->value(), true);

    // this makes the background of the container of the color bar transparent
    ui->colorBar->setAttribute(Qt::WA_OpaquePaintEvent, false);
    ui->colorBar->setBackground(QBrush(QColor(255,255,255,0)));

    statusBar()->addWidget(statusLabel);

    on_angleSpin_editingFinished(); // this is just to update the axes image...

    // connect al the slots to update the strain images
    connect(ui->colorBar, SIGNAL(limitsChanged(double, bool)), ui->exxPlot, SLOT(SetColorLimits(double, bool)));
    connect(ui->colorBar, SIGNAL(limitsChanged(double, bool)), ui->exyPlot, SLOT(SetColorLimits(double, bool)));
    connect(ui->colorBar, SIGNAL(limitsChanged(double, bool)), ui->eyxPlot, SLOT(SetColorLimits(double, bool)));
    connect(ui->colorBar, SIGNAL(limitsChanged(double, bool)), ui->eyyPlot, SLOT(SetColorLimits(double, bool)));

    connect(ui->colorBar, SIGNAL(mapChanged(QCPColorGradient, bool)), ui->exxPlot, SLOT(SetColorMap(QCPColorGradient, bool)));
    connect(ui->colorBar, SIGNAL(mapChanged(QCPColorGradient, bool)), ui->exyPlot, SLOT(SetColorMap(QCPColorGradient, bool)));
    connect(ui->colorBar, SIGNAL(mapChanged(QCPColorGradient, bool)), ui->eyxPlot, SLOT(SetColorMap(QCPColorGradient, bool)));
    connect(ui->colorBar, SIGNAL(mapChanged(QCPColorGradient, bool)), ui->eyyPlot, SLOT(SetColorMap(QCPColorGradient, bool)));

    fftw_init_threads();
    fftw_plan_with_nthreads(omp_get_max_threads());
    Eigen::setNbThreads(omp_get_max_threads()); // Not needed, just wanted to be verbose
}


MainWindow::~MainWindow()
{
    DisconnectAll();
    fftw_cleanup_threads();
    delete ui;
}

void MainWindow::updateStatusBar(QString message)
{
    statusLabel->setText(message);
}

void MainWindow::on_actionOpen_triggered()
{
    if (haveStrains)
    {
        auto reply = QMessageBox::question(this, tr("GPA"), tr("This will clear previous strain measurements.\nAre you OK with this?"), QMessageBox::No | QMessageBox::Yes);
        if (reply == QMessageBox::No)
            return;
    }

    haveImage = false;
    QSettings settings;


    QString fileName = QFileDialog::getOpenFileName(this, "Open File", settings.value("dialog/currentPath").toString(), "All supported (*.dm3 *.dm4 *.tif);; Digital micrograph (*.dm3 *.dm4);; TIFF (*.tif)");

    if (fileName.isNull())
        return;

    DisconnectAll();
    ClearImages();

    QFileInfo temp_file(fileName);

    settings.setValue("dialog/currentPath", temp_file.path());

    bool success = false;
    //QString ext = temp_file.suffix();
    if (temp_file.suffix() == "dm3" || temp_file.suffix() == "dm4")
#ifdef _WIN32
        success = openDM(fileName.toStdWString());
#else
        success = openDM(fileName.toStdString());
#endif
    if (temp_file.suffix() == "tif")
#ifdef _WIN32
        success = openTIFF(fileName.toStdWString());
#else
        success = openTIFF(fileName.toStdString());
#endif

    if (!success)
        return;

    showNewImageAndFFT(original_image);

    // clear plots of previous data
    ui->actionHann->setChecked(false);
    ui->menuExportAll->setEnabled(false);
    ui->menuExportStrains->setEnabled(false);
    haveStrains = false;

    ui->tabWidget->setCurrentIndex(0);
}

#ifdef _WIN32
bool MainWindow::openDM(std::wstring filename)
{
    std::shared_ptr<DMRead::DMReader> dmFile;
    try {
        dmFile = std::make_shared<DMRead::DMReader>(filename);
    } catch (const std::exception& e) {
        QMessageBox::information(this, tr("File open"), tr(e.what()), QMessageBox::Ok);
        return false;
    }

    return openDMProper(dmFile);
}
#endif

bool MainWindow::openDM(std::string filename)
{
    std::shared_ptr<DMRead::DMReader> dmFile;
    try {
        dmFile = std::make_shared<DMRead::DMReader>(filename);
    } catch (const std::exception& e) {
        QMessageBox::information(this, tr("File open"), tr(e.what()), QMessageBox::Ok);
        return false;
    }
    return openDMProper(dmFile);
}

bool MainWindow::openDMProper(std::shared_ptr<DMRead::DMReader> dmFile)
{
    // something to print tags?
    // dmFile->printTags();

    // get image data first as this catches some errors in a more sensible way
    // (e.g. binary images have no dimensions somehow....
    std::vector<double> image;
    try {
        image = dmFile->getImage();
    } catch (const std::exception& e) {
        QMessageBox::information(this, tr("File open"), tr(e.what()), QMessageBox::Ok);
        return false;
    }

    // get important image info
    int nx, ny, nz;
    try {
        nx = dmFile->getX();
        ny = dmFile->getY();
        nz = dmFile->getZ();
    } catch (const std::exception& e) {
        QMessageBox::information(this, tr("File open"), tr("Image dimensions not found."), QMessageBox::Ok);
        return false;
    }

    if (nx < 3 || ny < 3)
    {
        QMessageBox::information(this, tr("File open"), tr("Image too small."), QMessageBox::Ok);
        return false;
    }

    dmFile->close();

    // image is complex for FFTing later
    std::vector<Eigen::MatrixXcd> complexImage(nz, Eigen::MatrixXcd(ny, nx));

    #pragma omp parallel for
    for (int k = 0; k < nz; ++k ) {
        for (int i = 0; i < nx * ny; ++i)
            complexImage[k](i) = image[k * (nx * ny) + i];

        complexImage[k] = complexImage[k].colwise().reverse().eval();
    }

    image.clear();

    // set original image so we may reset to it later
    original_image = complexImage;
    return true;
}

#ifdef _WIN32
bool MainWindow::openTIFF(std::wstring filename)
{
    TIFFSetWarningHandler(nullptr);
    TIFF* tif = TIFFOpenW(filename.c_str(), "r");

    return openTIFFProper(tif);
}
#endif

bool MainWindow::openTIFF(std::string filename)
{
    TIFFSetWarningHandler(nullptr);
    TIFF* tif = TIFFOpen(filename.c_str(), "r");

    return openTIFFProper(tif);
}

bool MainWindow::openTIFFProper(TIFF* tif)
{
    if (tif == nullptr)
    {
        QMessageBox::information(this, tr("File open"), tr("Error opening TIFF"), QMessageBox::Ok);
        return false;
    }

    // this is defaulting to 1 according to: https://www.awaresystems.be/imaging/tiff/tifftags/samplesperpixel.html
    uint32 samples = 1;
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples);

    if (samples != 1)
    {
        QMessageBox::information(this, tr("File open"), tr("TIFF must be greyscale"), QMessageBox::Ok);
        return false;
    }

    uint16 format = SAMPLEFORMAT_UINT;
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &format);

    uint16 bitsper = 1;
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsper);

    if (format == SAMPLEFORMAT_UINT)
    {
        if (bitsper == 8)
            readTiffData<uint8>(tif);
        else if (bitsper == 16)
            readTiffData<uint16>(tif);
        else if(bitsper == 32)
            readTiffData<uint32>(tif);
        else if (bitsper == 64)
            readTiffData<uint64>(tif);
    }
    else if (format == SAMPLEFORMAT_INT)
    {
        if (bitsper == 8)
            readTiffData<int8>(tif);
        else if (bitsper == 16)
            readTiffData<int16>(tif);
        else if(bitsper == 32)
            readTiffData<int32>(tif);
        else if (bitsper == 64)
            readTiffData<int64>(tif);
    }
    else if (format == SAMPLEFORMAT_IEEEFP)
    {
        if(bitsper == 32)
            readTiffData<float>(tif);
        else if (bitsper == 64)
            readTiffData<double>(tif);
    }
    else
    {
        QMessageBox::information(this, tr("File open"), tr("Unsupported TIFF format"), QMessageBox::Ok);
        return false;
    }

    TIFFClose(tif);

    return true;
}

void MainWindow::showNewImageAndFFT(std::vector<Eigen::MatrixXcd> &image, unsigned int slice)
{
    GPAstrain = std::make_unique<GPA>(GPA(image[slice]));

    showImageAndFFT();
}

void MainWindow::showImageAndFFT(bool rePlot)
{
    ui->imagePlot->clearImage(false);
    ui->fftPlot->clearImage(false);

    // show real space image
    try
    {
        ui->imagePlot->SetImage(*(GPAstrain->getImage()), ShowComplex::Real, rePlot);
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(nullptr,"Error", e.what());
        return;
    }

    // show reciprocal space image
    try
    {
        ui->fftPlot->SetImage(*(GPAstrain->getFFT()), ShowComplex::PowerSpectrum, rePlot);
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(nullptr,"Error", e.what());
        return;
    }

    haveImage = true;
    ui->actionHann->setEnabled(true);
    ui->actionGPA->setEnabled(true);
}

void MainWindow::on_actionGPA_triggered()
{
    if (!haveImage)
        return;

    DisconnectAll();
    ClearImages();

    ui->tabWidget->setCurrentIndex(0);

    // remove annotation from other, old strain analyses
    ui->fftPlot->clearAllItems();
    ui->fftPlot->replot();

    minGrad = GPAstrain->getGVectors();

    AcceptGVector();
}

void MainWindow::AcceptGVector()
{
    bool happy = false;
    // I hate myself for this loop, but it is to avoid recursion with the manual entry
    while (!happy)
    {
        ui->fftPlot->DrawCircle(0, 0, Qt::red, QBrush(Qt::NoBrush), minGrad, Qt::DotLine);
        ui->fftPlot->DrawCircle(0, 0, Qt::red, QBrush(Qt::NoBrush), minGrad / 2);
        ui->fftPlot->DrawCircle(0, 0);

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("GPA"));
        msgBox.setText(tr("Accept mask size?"));
        msgBox.setInformativeText(tr("(Solid Line)"));

        QAbstractButton *btnManual = msgBox.addButton(tr("Manual"), QMessageBox::ActionRole);
        QAbstractButton *btnRadius = msgBox.addButton(tr("Smallest g"), QMessageBox::ActionRole);
        msgBox.addButton(tr("Yes"), QMessageBox::YesRole);

        msgBox.setIcon(QMessageBox::Question);
        msgBox.exec();
        //auto reply = QMessageBox::question(this, tr("GPA"), tr("Accept mask size?\n(The dashed circle should touch the smallest g-vector)"), QMessageBox::No | QMessageBox::Yes);

        ui->fftPlot->clearAllItems();
        ui->fftPlot->replot();

        if (msgBox.clickedButton() == btnRadius)
        {
            happy = true;
            updateStatusBar("Select the smallest g-vector on the FFT");
            if (!minimalDialogs)
                QMessageBox::information(this, tr("GPA"), tr("Select the smallest g-vector on the FFT"), QMessageBox::Ok);

            connect(ui->fftPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(clickGVector(QMouseEvent*)));
        }
        else if (msgBox.clickedButton() == btnManual)
        {
            updateStatusBar("Enter the desired mask size");
            minGrad = QInputDialog::getDouble(this, tr("GPA"), tr("Mask size (3σ):"), minGrad, 0.0, 2147483647.0, 2);
        }
        else
        {
            // we are happy with our mask size
            happy = true;
            // we will be selecting our first G-vector
            phaseSelection = 0;

            // check if we want to reuse out first G-vector!
            bool reused_g = false;
            if (lastG1.size() == 2 && reuseGs) {
                auto ret = QMessageBox::information(this, tr("GPA"), tr("Reuse first G-vector?"), QMessageBox::Yes | QMessageBox::No);

                if (ret == QMessageBox::Yes) {
                    processBraggClick(lastG1[0], lastG1[1]);
                    reused_g = true; // we have reused it!
                }
            }

            if (!reused_g) { // because we have not reused a G-vector, need to get one
                updateStatusBar("Select a g-vector on the FFT");
                if (!minimalDialogs)
                    QMessageBox::information(this, tr("GPA"), tr("Select a g-vector on the FFT"), QMessageBox::Ok);

                connect(ui->fftPlot, SIGNAL(mousePress(QMouseEvent * )), this, SLOT(clickBraggSpot(QMouseEvent * )));
            }
        }
    }
}

void MainWindow::clickGVector(QMouseEvent *event) {

    double x = ui->fftPlot->xAxis->pixelToCoord(event->pos().x());
    double y = ui->fftPlot->yAxis->pixelToCoord(event->pos().y());

    if(!ui->fftPlot->inAxis(x, y))
        return;

    disconnect(ui->fftPlot, SIGNAL(mousePress(QMouseEvent*)), nullptr, nullptr);
    updateStatusBar("--");

    minGrad = std::sqrt(x*x+y*y);

    ui->fftPlot->DrawCircle(0, 0, Qt::red, QBrush(Qt::NoBrush), minGrad, Qt::DotLine);
    ui->fftPlot->DrawCircle(0, 0, Qt::red, QBrush(Qt::NoBrush), minGrad / 2);
    ui->fftPlot->DrawCircle(0, 0);

    AcceptGVector();
}

void MainWindow::clickBraggSpot(QMouseEvent *event) {
    // get coords of click
    double x = ui->fftPlot->xAxis->pixelToCoord(event->pos().x());
    double y = ui->fftPlot->yAxis->pixelToCoord(event->pos().y());

    if (!ui->fftPlot->inAxis(x, y))
        return;

    disconnect(ui->fftPlot, SIGNAL(mousePress(QMouseEvent*)), nullptr, nullptr);
    updateStatusBar("--");

    processBraggClick(x, y);
}

void MainWindow::processBraggClick(double x, double y) {
    // calculate optimal sigma [REF]
    _sig = minGrad/(2*3);

    QColor phaseCol;

    if (phaseSelection == 1)
        phaseCol = Qt::blue;
    else
        phaseCol = Qt::red;

    ui->fftPlot->DrawCircle(x, y, phaseCol, QBrush(phaseCol));
    ui->fftPlot->DrawCircle(x, y, phaseCol, QBrush(Qt::NoBrush), 3*_sig); // assume here that 3sigma is the ask radius.

    GPAstrain->calculatePhase(phaseSelection, x, y, _sig);

    // save these so we could use them later
    if (phaseSelection == 0) {
        lastG1 = {x, y};
    } else if (phaseSelection == 1) {
        lastG2 = {x, y};
    }

    Eigen::MatrixXd ph = GPAstrain->getPhase(phaseSelection)->getWrappedPhase();

    try
    {
        ui->imagePlot->SetImage(ph);
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(nullptr,"Error", e.what());
        return;
    }

    auto ret = QMessageBox::information(this, tr("Refine"), tr("Do you want to  refine this g-vector"), QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes)
        selectRefineArea();
    else
        continuePhase();
}


void MainWindow::continuePhase()
{
    // reset the image
    try
    {
        ui->imagePlot->SetImage(*(GPAstrain->getImage()), ShowComplex::Amplitude);
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(nullptr,"Error", e.what());
        return;
    }

    if (phaseSelection == 0)
    { // if this is the first phase, select another.
        ++phaseSelection;

        bool reused_g = false;
        if (lastG2.size() == 2 && reuseGs) {
            auto ret = QMessageBox::information(this, tr("GPA"), tr("Reuse second G-vector?"), QMessageBox::Yes | QMessageBox::No);

            if (ret == QMessageBox::Yes) {
                processBraggClick(lastG2[0], lastG2[1]);
                reused_g = true; // we have reused it!
            }
        }

        if (!reused_g) {
            updateStatusBar("Select another g-vector on the FFT");
            if (!minimalDialogs)
                QMessageBox::information(this, tr("GPA"), tr("Select another g-vector on the FFT"), QMessageBox::Ok);
            connect(ui->fftPlot, SIGNAL(mousePress(QMouseEvent * )), this, SLOT(clickBraggSpot(QMouseEvent * )));
        }
    }
    else
    {
        getStrains();
    }

}


void MainWindow::selectRefineArea()
{
    if (phaseSelection != 0 && xCorner.size() == 2)
    {
        double top = std::max(yCorner[0], yCorner[1]);
        double bottom = std::min(yCorner[0], yCorner[1]);

        double right = std::max(xCorner[0], xCorner[1]);
        double left = std::min(xCorner[0], xCorner[1]);

        ui->imagePlot->DrawRectangle(top, left, bottom, right);

        auto rect = QMessageBox::information(this, tr("Refine"), tr("Use same area?"), QMessageBox::Yes | QMessageBox::No);
        if (rect == QMessageBox::Yes)
        {
            xCorner.clear();
            yCorner.clear();
            doRefinement(top, left, bottom, right);
        }
        else
        {
            if(!minimalDialogs)
                QMessageBox::information(this, tr("GPA"), tr("Select corners of refinement area on phase"), QMessageBox::Ok);
            ui->imagePlot->clearAllItems();
            ui->imagePlot->replot();
            xCorner.clear();
            yCorner.clear();
            connect(ui->imagePlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(clickRectCorner(QMouseEvent*)));
        }

    }
    else
    {
        if(!minimalDialogs)
            QMessageBox::information(this, tr("GPA"), tr("Select corners of refinement area on phase"), QMessageBox::Ok);
        xCorner.clear();
        yCorner.clear();
        connect(ui->imagePlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(clickRectCorner(QMouseEvent*)));
    }
}


void MainWindow::clickRectCorner(QMouseEvent *event)
{
    //check vector sizes etc for error?

    double x = ui->imagePlot->xAxis->pixelToCoord(event->pos().x());
    double y = ui->imagePlot->yAxis->pixelToCoord(event->pos().y());

    // if coords outside image, ignore event
    if (!ui->imagePlot->inAxis(x, y))
        return;

    ui->imagePlot->DrawCircle(x, y);

    xCorner.push_back(x);
    yCorner.push_back(y);

    if(xCorner.size() == 2)
    {
        disconnect(ui->imagePlot, SIGNAL(mousePress(QMouseEvent*)), nullptr, nullptr);

        double top = std::max(yCorner[0], yCorner[1]);
        double bottom = std::min(yCorner[0], yCorner[1]);

        double right = std::max(xCorner[0], xCorner[1]);
        double left = std::min(xCorner[0], xCorner[1]);

        ui->imagePlot->DrawRectangle(top, left, bottom, right);

        auto rect = QMessageBox::information(this, tr("Refine"), tr("Accept area?"), QMessageBox::Yes | QMessageBox::No);

        if (rect == QMessageBox::Yes)
        {
            ui->imagePlot->clearAllItems();
            ui->imagePlot->replot();
            doRefinement(top, left, bottom, right);
        }
        else
        {
            // clear figure
            ui->imagePlot->clearAllItems();
            ui->imagePlot->replot();
            xCorner.clear();
            yCorner.clear();
            selectRefineArea();
        }

    }
}


void MainWindow::doRefinement(double top, double left, double bottom, double right)
{
    int rowmid = static_cast<int>(GPAstrain->getImage()->rows() / 2);
    int colmid = static_cast<int>(GPAstrain->getImage()->cols() / 2);

    int t = static_cast<int>(top) + rowmid;
    int b = static_cast<int>(bottom) + rowmid;
    int l = static_cast<int>(left) + colmid;
    int r = static_cast<int>(right) + colmid;

    GPAstrain->getPhase(phaseSelection)->refinePhase(t, l, b, r);

    ui->fftPlot->clearAllItems();

    if (phaseSelection == 0)
    {
        auto GVecPx = GPAstrain->getPhase(phaseSelection)->getGVectorPixels();
        ui->fftPlot->DrawCircle(GVecPx.x, GVecPx.y, Qt::red, QBrush(Qt::red));
        ui->fftPlot->DrawCircle(GVecPx.x, GVecPx.y, Qt::red, QBrush(Qt::NoBrush), 3*_sig);
    }
    else
    {
        auto GVecPx1 = GPAstrain->getPhase(0)->getGVectorPixels();
        ui->fftPlot->DrawCircle(GVecPx1.x, GVecPx1.y, Qt::red, QBrush(Qt::red));
        ui->fftPlot->DrawCircle(GVecPx1.x, GVecPx1.y, Qt::red, QBrush(Qt::NoBrush), 3*_sig);

        auto GVecPx2 = GPAstrain->getPhase(phaseSelection)->getGVectorPixels();
        ui->fftPlot->DrawCircle(GVecPx2.x, GVecPx2.y, Qt::blue, QBrush(Qt::blue));
        ui->fftPlot->DrawCircle(GVecPx2.x, GVecPx2.y, Qt::blue, QBrush(Qt::NoBrush), 3*_sig);
    }

    Eigen::MatrixXd ph = GPAstrain->getPhase(phaseSelection)->getWrappedPhase();

    try
    {
        ui->imagePlot->SetImage(ph);
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(nullptr, "Error", e.what());
        return;
    }

    auto rect = QMessageBox::information(this, tr("Refine"), tr("Refine again?"), QMessageBox::Yes | QMessageBox::No);

    if (rect == QMessageBox::No)
    {
        continuePhase();
    }
    else
    {
        xCorner.clear();
        yCorner.clear();
        connect(ui->imagePlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(clickRectCorner(QMouseEvent*)));
    }
}

// probably poorly named since it does so much more
void MainWindow::getStrains(bool showTab, bool rePlot)
{
    updateStatusBar("GPA completed!");

    if (showTab)
        ui->tabWidget->setCurrentIndex(1);

    double angle = ui->angleSpin->value();
    std::string mode = ui->resultModeBox->currentText().toStdString();

    GPAstrain->calculateDistortion(angle, mode);

    if (mode == "Distortion" || mode == "Strain")
    {
        ui->exxPlot->SetImage(*(GPAstrain->getExx()), false);
        ui->exyPlot->SetImage(*(GPAstrain->getExy()), false);
        ui->eyxPlot->SetImage(*(GPAstrain->getEyx()), false);
        ui->eyyPlot->SetImage(*(GPAstrain->getEyy()), false);
    }
    else if (mode == "Rotation")
    {
        ui->exxPlot->clearImage();
        ui->exyPlot->SetImage(*(GPAstrain->getExy()), false);
        ui->eyxPlot->SetImage(*(GPAstrain->getEyx()), false);
        ui->eyyPlot->clearImage();
    }
    else if (mode == "Dilitation")
    {
        ui->exxPlot->SetImage(*(GPAstrain->getExx()), false);
        ui->exyPlot->clearImage();
        ui->eyxPlot->clearImage();
        ui->eyyPlot->clearImage();
    }

    haveStrains = true;

    double lim =  ui->colorBar->GetLimits().upper;

    emit ui->colorBar->limitsChanged(lim, rePlot);

    QCPColorGradient map = ui->colorBar->GetColorMap();

    emit ui->colorBar->mapChanged(map, rePlot);

    updateOtherPlot(ui->leftCombo->currentIndex(), 0, rePlot);
    updateOtherPlot(ui->rightCombo->currentIndex(), 1, rePlot);

    ui->menuExportAll->setEnabled(true);
    ui->menuExportStrains->setEnabled(true);
}

void MainWindow::on_leftCombo_currentIndexChanged(int index)
{
    if (haveStrains)
        updateOtherPlot(index, 0);
}

void MainWindow::on_rightCombo_currentIndexChanged(int index)
{
    if (haveStrains)
        updateOtherPlot(index, 1);
}

void MainWindow::updateOtherPlot(int index, int side, bool rePlot)
{
    ImagePlot* image;
    if (side == 0)
        image = ui->leftOtherPlot;
    else if (side == 1)
        image = ui->rightOtherPlot;
    else
        return;

    switch(index)
    {
    case 0 : image->SetImage(GPAstrain->getPhase(side)->getGaussianMask(), rePlot); break;
    case 1 : image->SetImage(GPAstrain->getPhase(side)->getMaskedFFT(), ShowComplex::PowerSpectrum, rePlot); break;
    case 2 : image->SetImage(GPAstrain->getPhase(side)->getBraggImage(), rePlot); break;
    case 3 : image->SetImage(GPAstrain->getPhase(side)->getRawPhase(), rePlot); break;
    case 4 : image->SetImage(GPAstrain->getPhase(side)->getPhase(), rePlot); break;
    case 5 : image->SetImage(GPAstrain->getPhase(side)->getWrappedPhase(), rePlot); break;
    default : break;
    }
    if (index == 6 || index== 7)
    {
        auto imSize = GPAstrain->getSize();
        Eigen::MatrixXcd dx(imSize.y, imSize.x);
        Eigen::MatrixXcd dy(imSize.y, imSize.x);
        GPAstrain->getPhase(side)->getDifferential(dx, dy);
        if (index == 6)
            image->SetImage(dx, ShowComplex::Real, rePlot);
        else
            image->SetImage(dy, ShowComplex::Real, rePlot);
    }
    else if (index == -1)
    {
        image->clearImage();
    }
}

void MainWindow::on_limitsSpin_editingFinished()
{
    double lim = ui->limitsSpin->value();
    ui->colorBar->SetLimits(lim, true);
}

void MainWindow::on_colourMapBox_currentIndexChanged(const QString &Map)
{
    ui->colorBar->SetColorMap(Map, true);
}

void MainWindow::on_actionMinimal_dialogs_triggered()
{
    minimalDialogs = ui->actionMinimal_dialogs->isChecked();
    QSettings settings;
    settings.setValue("dialog/minimal", minimalDialogs);
}

void MainWindow::on_actionReuse_gs_triggered()
{
    reuseGs = ui->actionReuse_gs->isChecked();
    QSettings settings;
    settings.setValue("dialog/reuseGs", reuseGs);
}

void MainWindow::on_actionHann_triggered()
{
    if(!haveImage)
        return;

    if (haveStrains)
    {
        auto reply = QMessageBox::question(this, tr("GPA"), tr("This will clear previous strain measurements.\nAre you OK with this?"), QMessageBox::No | QMessageBox::Yes);
        if (reply == QMessageBox::No)
        {
            ui->actionHann->setChecked(!ui->actionHann->isChecked());
            return;
        }
    }

    DisconnectAll();
    ClearImages();

    GPAstrain->setDoHann(ui->actionHann->isChecked());

    showImageAndFFT();
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::ExportAll(int choice) {
    if (!haveStrains)
        return;

    QSettings settings;

    // get path
    QString fileDir = QFileDialog::getExistingDirectory(this, "Save directory",
                                                        settings.value("dialog/currentSavePath").toString(),
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (fileDir.isEmpty())
        return;

    settings.setValue("dialog/currentSavePath", fileDir);

    auto reply = QMessageBox::No;
    if (original_image.size() > 1)
        reply = QMessageBox::question(this, tr("GPA"), tr("Export for all slices in stack?"), QMessageBox::No | QMessageBox::Yes);

    if (reply == QMessageBox::Yes)
    {
        int nw = static_cast<int>(std::floor(std::log10(original_image.size()) + 1));

        // here decide if stack is to be done
        for (int i = 0; i < original_image.size(); ++i) {
            GPAstrain->updateImage(original_image[i]);
            showImageAndFFT(false);
            getStrains(false, false);

            ExportAllSlice(fileDir, choice, QString::number(i).rightJustified(nw, '0'), i == 0);
        }

        // reset the current image back to the original visible one
        GPAstrain->updateImage(original_image[0]);
        showImageAndFFT(true);
        getStrains(false);
    }
    else
    {
        ExportAllSlice(fileDir, choice, "", true);
    }

}

void MainWindow::ExportAllSlice(QString fileDir, int choice, QString prefix, bool do_colbar)
{
    if (!prefix.isEmpty())
        prefix += " ";

    ui->imagePlot->ExportSelector(fileDir, prefix + "image", choice);
    ui->fftPlot->ExportSelector(fileDir, prefix + "FFT", choice);

    std::string mode = ui->resultModeBox->currentText().toStdString();

    if (mode == "Distortion")
    {
        ui->exxPlot->ExportSelector(fileDir, prefix + "exx", choice);
        ui->exyPlot->ExportSelector(fileDir, prefix + "exy", choice);
        ui->eyxPlot->ExportSelector(fileDir, prefix + "eyx", choice);
        ui->eyyPlot->ExportSelector(fileDir, prefix + "eyy", choice);
    }
    else if(mode == "Strain")
    {
        ui->exxPlot->ExportSelector(fileDir, prefix + "epsxx", choice);
        ui->exyPlot->ExportSelector(fileDir, prefix + "epsxy", choice);
        ui->eyyPlot->ExportSelector(fileDir, prefix + "epsyy", choice);
    }
    else if(mode == "Rotation")
    {
        ui->exyPlot->ExportSelector(fileDir, prefix + "wxy", choice);
        ui->eyxPlot->ExportSelector(fileDir, prefix + "wyx", choice);
    }
    else if(mode == "Dilitation")
    {
        ui->exxPlot->ExportSelector(fileDir, prefix + "Dilitation", choice);
    }

    if (choice == 0 && do_colbar)
        ui->colorBar->ExportImage(fileDir, "ColourBar");

    // need to loop through and show images before exporting them from same plot
    for(int i = 0; i < ui->leftCombo->count(); ++i)
    {
        updateOtherPlot(i, 0, false);
        updateOtherPlot(i, 1, false);

        ui->leftOtherPlot->ExportSelector(fileDir, prefix + "Phase 1 " + ui->leftCombo->itemText(i).remove("/"), choice);
        ui->rightOtherPlot->ExportSelector(fileDir, prefix + "Phase 2 " + ui->rightCombo->itemText(i).remove("/"), choice);
    }
    // If I don't clear first it makes the image distorted
    //ui->leftOtherPlot->clearImage();
    //ui->rightOtherPlot->clearImage();
    //updateOtherPlot(ui->leftCombo->currentIndex(), 0);
    //updateOtherPlot(ui->rightCombo->currentIndex(), 1);
}

void MainWindow::ExportStrains(int choice) {
    if (!haveStrains)
        return;

    QSettings settings;

    // get path
    QString fileDir = QFileDialog::getExistingDirectory(this, "Save directory",
                                                        settings.value("dialog/currentSavePath").toString(),
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (fileDir.isEmpty())
        return;

    settings.setValue("dialog/currentSavePath", fileDir);

    auto reply = QMessageBox::No;
    if (original_image.size() > 1)
        reply = QMessageBox::question(this, tr("GPA"), tr("Export for all slices in stack?"), QMessageBox::No | QMessageBox::Yes);


    if (reply == QMessageBox::Yes)
    {
        int nw = static_cast<int>(std::floor(std::log10(original_image.size()) + 1));

        // here decide if stack is to be done
        for (int i = 0; i < original_image.size(); ++i) {
            GPAstrain->updateImage(original_image[i]);
            showImageAndFFT(false);
            getStrains(false, false);

            ExportStrainsSlice(fileDir, choice, QString::number(i).rightJustified(nw, '0'), i == 0);
        }

        // reset the current image back to the original visible one
        GPAstrain->updateImage(original_image[0]);
        showImageAndFFT(true);
        getStrains(false);
    }
    else
    {
        ExportStrainsSlice(fileDir, choice, "", true);
    }
}

void MainWindow::ExportStrainsSlice(QString fileDir, int choice, QString prefix, bool do_colbar)
{
    if (!prefix.isEmpty())
        prefix += " ";

    std::string mode = ui->resultModeBox->currentText().toStdString();

    if (mode == "Distortion")
    {
        ui->exxPlot->ExportSelector(fileDir, prefix + "exx", choice);
        ui->exyPlot->ExportSelector(fileDir, prefix + "exy", choice);
        ui->eyxPlot->ExportSelector(fileDir, prefix + "eyx", choice);
        ui->eyyPlot->ExportSelector(fileDir, prefix + "eyy", choice);
    }
    else if(mode == "Strain")
    {
        ui->exxPlot->ExportSelector(fileDir, prefix + "epsxx", choice);
        ui->exyPlot->ExportSelector(fileDir, prefix + "epsxy", choice);
        ui->eyxPlot->ExportSelector(fileDir, prefix + "epsyx", choice);
        ui->eyyPlot->ExportSelector(fileDir, prefix + "epsyy", choice);
    }
    else if(mode == "Rotation")
    {
        ui->exyPlot->ExportSelector(fileDir, prefix + "wxy", choice);
        ui->eyxPlot->ExportSelector(fileDir, prefix + "wyx", choice);
    }
    else if(mode == "Dilitation")
    {
        ui->exxPlot->ExportSelector(fileDir, prefix + "Dilitation", choice);
    }

    if (choice == 0 && do_colbar)
        ui->colorBar->ExportImage(fileDir, "ColourBar");
}

void MainWindow::DisconnectAll()
{
    // stop current GPA if in progress
//    disconnect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickBraggSpot);
//    disconnect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickGVector);
//    disconnect(ui->imagePlot, &ImagePlot::mousePress, this, &MainWindow::clickRectCorner);

    disconnect(ui->fftPlot, SIGNAL(mousePress(QMouseEvent*)), nullptr, nullptr);
    disconnect(ui->imagePlot, SIGNAL(mousePress(QMouseEvent*)), nullptr, nullptr);
}

void MainWindow::ClearImages()
{
    haveStrains = false;
    ui->exxPlot->clearImage();
    ui->exyPlot->clearImage();
    ui->eyxPlot->clearImage();
    ui->eyyPlot->clearImage();

    ui->rightOtherPlot->clearImage();
    ui->leftOtherPlot->clearImage();
}

void MainWindow::on_angleSpin_editingFinished()
{
    // OK....
    // This first part is very messy. But bear with me.
    // The axes is actually an SVG file kept int the resource file "axesresource.qrc"

    // simple, get the angle!
    double angle = ui->angleSpin->value();

    // These are to account for the size of the text
    // there might be a better way using rects and setting the alignment to centre
    // but I can't be botthered to work it out
    QPointF ysz(-3, 4.5);
    QPointF xsz(-3, 3);
    // This is the size of our area to draw in
    QSize size(100, 100);
    // This is the pixmap that will eventually be shown, we start with it transparent
    QPixmap newmap(size);
    newmap.fill(Qt::transparent);
    // this is the midpoint of our image (so we can rotate around it)
    QPointF mid(size.height()/2., size.width()/2.);
    // this is the position of our labels
    QPointF xp(65, 65);
    QPointF yp(35, 35);

    // here we rotate these positions about the mid point
    xp = xp - mid;
    yp = yp - mid;
    QTransform temp;
    QTransform tform = temp.rotate(-angle);

    xp = tform.map(xp);
    yp = tform.map(yp);
    xp = xp + mid;
    yp = yp + mid;

    // set the font and colours
    QFont tFont("Arial", 12, QFont::Normal);
    QPen xPen(QColor("#E30513"));
    QPen yPen(QColor("#008D36"));

    // here we load the svg from the resource file
    QSvgRenderer renderer(QString(":/Images/axes.svg"));
    auto p = new QPainter(&newmap);
    //draw the texts (we have rotated the POSITION before)
    p->setFont(tFont);
    p->setPen(xPen);
    p->drawText(xp + xsz, "x");
    p->setPen(yPen);
    p->drawText(yp + ysz, "y");
    // now rotate the svg about hte mid point and draw it
    p->translate(size.height()/2.,size.height()/2.);
    p->rotate(-angle);
    p->translate(-size.height()/2.,-size.height()/2.);
    renderer.render(p);
    // done, phew...
    p->end();

    // finally set it
    ui->lblAxes->setPixmap(newmap);

    // now do the calculation if we need to
    if(!haveStrains || lastAngle == angle)
        return;

    lastAngle = angle;

    getStrains();
}

void MainWindow::on_resultModeBox_currentIndexChanged(const QString &mode)
{
    // set the labels on the image
    if (mode == "Distortion")
    {
        ui->exxLabel->setText("e<sub>xx</sub>");
        ui->exyLabel->setText("e<sub>xy</sub>");
        ui->eyxLabel->setText("e<sub>yx</sub>");
        ui->eyyLabel->setText("e<sub>yy</sub>");
        ui->exxLabel->setVisible(true);
        ui->exyLabel->setVisible(true);
        ui->eyxLabel->setVisible(true);
        ui->eyyLabel->setVisible(true);
    }
    else if(mode == "Strain")
    {
        ui->exxLabel->setText("ε<sub>xx</sub>");
        ui->exyLabel->setText("ε<sub>xy</sub>");
        ui->eyxLabel->setText("ε<sub>yx</sub>");
        ui->eyyLabel->setText("ε<sub>yy</sub>");
        ui->exxLabel->setVisible(true);
        ui->exyLabel->setVisible(true);
        ui->eyxLabel->setVisible(true);
        ui->eyyLabel->setVisible(true);
    }
    else if(mode == "Rotation")
    {
        ui->exyLabel->setText("ω<sub>xy</sub>");
        ui->eyxLabel->setText("ω<sub>yx</sub>");
        ui->exxLabel->setVisible(false);
        ui->exyLabel->setVisible(true);
        ui->eyxLabel->setVisible(true);
        ui->eyyLabel->setVisible(false);
    }
    else if(mode == "Dilitation")
    {
        ui->exxLabel->setVisible(true);
        ui->exyLabel->setVisible(false);
        ui->eyxLabel->setVisible(false);
        ui->eyyLabel->setVisible(false);
        ui->exxLabel->setText("Δ");
    }

    if(!haveStrains)
        return;

    getStrains();
}

void MainWindow::on_actionAbout_triggered() {
    show_version_dialog();
}
