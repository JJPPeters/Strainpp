#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "dmreader.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QCoreApplication::setOrganizationName("PetersSoft");
    QCoreApplication::setApplicationName("Strain++");

    QSettings settings;
    if (!settings.contains("dialog/currentPath"))
        settings.setValue("dialog/currentPath", QStandardPaths::HomeLocation);
    if (!settings.contains("dialog/currentSavePath"))
        settings.setValue("dialog/currentSavePath", QStandardPaths::HomeLocation);

    ui->setupUi(this);

    setWindowTitle("Strain++");

    //add lebel to  status bar (can't be done with designer)
    statusLabel = new QLabel("--");
    statusLabel->setContentsMargins(10,0,0,0);
    statusLabel->setSizePolicy(QSizePolicy::MinimumExpanding,
                               QSizePolicy::MinimumExpanding);

    ui->colorBar->SetColorMap(ui->colourMapBox->currentText());
    ui->colorBar->SetLimits(ui->limitsSpin->value());

    statusBar()->addWidget(statusLabel);


    // connect al the slots to update the strain images
    connect(ui->colorBar, SIGNAL(limitsChanged(double)), ui->exxPlot, SLOT(SetColorLimits(double)));
    connect(ui->colorBar, SIGNAL(limitsChanged(double)), ui->exyPlot, SLOT(SetColorLimits(double)));
    connect(ui->colorBar, SIGNAL(limitsChanged(double)), ui->eyxPlot, SLOT(SetColorLimits(double)));
    connect(ui->colorBar, SIGNAL(limitsChanged(double)), ui->eyyPlot, SLOT(SetColorLimits(double)));

    connect(ui->colorBar, SIGNAL(mapChanged(QCPColorGradient)), ui->exxPlot, SLOT(SetColorMap(QCPColorGradient)));
    connect(ui->colorBar, SIGNAL(mapChanged(QCPColorGradient)), ui->exyPlot, SLOT(SetColorMap(QCPColorGradient)));
    connect(ui->colorBar, SIGNAL(mapChanged(QCPColorGradient)), ui->eyxPlot, SLOT(SetColorMap(QCPColorGradient)));
    connect(ui->colorBar, SIGNAL(mapChanged(QCPColorGradient)), ui->eyyPlot, SLOT(SetColorMap(QCPColorGradient)));
  
}


MainWindow::~MainWindow()
{
    DisconnectAll();
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

    //QString ext = temp_file.suffix();
    if (temp_file.suffix() == "dm3" || temp_file.suffix() == "dm4")
        openDM(fileName.toStdString());
    if (temp_file.suffix() == "tif")
        openTIFF(fileName.toStdString());

    // clear plots of previous data
    ui->actionHann->setChecked(false);
    ui->menuExportAll->setEnabled(false);
    ui->menuExportStrains->setEnabled(false);
    haveStrains = false;

    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::openDM(std::string filename)
{
    DMRead::DMReader dmFile(filename);

    // get important inmage info
    int nx = dmFile.getX();
    int ny = dmFile.getY();
    int nz = dmFile.getZ();

    std::vector<double> image;
    if ( nz > 1)
        image = dmFile.getImage(0, nx*ny);
    else
        image = dmFile.getImage();

    dmFile.close();;

    // image is complex for FFTing later
    Eigen::MatrixXcd complexImage(ny, nx);

    for (int i = 0; i < complexImage.size(); ++i)
        complexImage(i) = image[i];

    complexImage = complexImage.colwise().reverse().eval();

    image.clear();

    // set original image so we may reset to it later
    original_image = complexImage;

    showImageAndFFT(complexImage);
}

void MainWindow::openTIFF(std::string filename)
{
    TIFFSetWarningHandler(NULL);
    TIFF* tif = TIFFOpen(filename.c_str(), "r");

    if (!tif)
    {
        QMessageBox::information(this, tr("File open"), tr("Error opening TIFF"), QMessageBox::Ok);
        return;
    }

    uint32 samples;
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples);

    if (samples != 1)
    {
        QMessageBox::information(this, tr("File open"), tr("TIFF must be greyscale"), QMessageBox::Ok);
        return;
    }

    uint16 format;
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &format);

    uint16 bitsper;
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
        return;
    }

    TIFFClose(tif);
}

void MainWindow::showImageAndFFT(Eigen::MatrixXcd &image)
{
    ui->imagePlot->clearImage(false);
    ui->fftPlot->clearImage(false);

    GPAstrain = std::make_unique<GPA>(GPA(image));

    // show real space image
    try
    {
        ui->imagePlot->SetImage(*(GPAstrain->getImage()), ShowComplex::Real);
    }
    catch (const std::exception& e)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error", e.what());
        return;
    }

    // show reciprocal space image
    try
    {
        ui->fftPlot->SetImage(*(GPAstrain->getFFT()), ShowComplex::PowerSpectrum);
    }
    catch (const std::exception& e)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error", e.what());
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

    ui->tabWidget->setCurrentIndex(0);

    // remove annotation from other strain analyses
    ui->fftPlot->clearItems();
    ui->fftPlot->replot();

    minGrad = GPAstrain->getGVectors();

    ui->fftPlot->DrawCircle(0, 0, Qt::red, QBrush(Qt::NoBrush), minGrad);
    ui->fftPlot->DrawCircle(0, 0);

    auto reply = QMessageBox::question(this, tr("GPA"), tr("Does the circle touch the smallest g-vector?"), QMessageBox::No | QMessageBox::Yes);

    ui->fftPlot->clearItems();
    ui->fftPlot->replot();

    if (reply == QMessageBox::No)
    {
        updateStatusBar("Select the smallest g-vector on the FFT");
        QMessageBox::information(this, tr("GPA"), tr("Select the smallest g-vector on the FFT"), QMessageBox::Ok);
        connect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickGVector);
    }
    else
    {
        phaseSelection = 0;
        updateStatusBar("Select a g-vector on the FFT");
        QMessageBox::information(this, tr("GPA"), tr("Select a g-vector on the FFT"), QMessageBox::Ok);

        connect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickBraggSpot);
    }
}

void MainWindow::clickGVector(QMouseEvent *event)
{
    disconnect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickGVector);
    updateStatusBar("--");

    double x = ui->fftPlot->xAxis->pixelToCoord(event->pos().x());
    double y = ui->fftPlot->yAxis->pixelToCoord(event->pos().y());

    minGrad = std::sqrt(x*x+y*y);

    ui->fftPlot->DrawCircle(0, 0, Qt::red, QBrush(Qt::NoBrush), minGrad);
    ui->fftPlot->DrawCircle(0, 0);

    auto reply = QMessageBox::question(this, tr("GPA"), tr("Accept this radius?"), QMessageBox::No | QMessageBox::Yes);

    ui->fftPlot->clearItems();
    ui->fftPlot->replot();

    if (reply == QMessageBox::No)
    {
        connect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickGVector);
        return;
    }
    else
    {
        phaseSelection = 0;
        updateStatusBar("Select a g-vector on the FFT");
        QMessageBox::information(this, tr("GPA"), tr("Select a g-vector on the FFT"), QMessageBox::Ok);

        connect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickBraggSpot);
    }
}

void MainWindow::clickBraggSpot(QMouseEvent *event)
{
    disconnect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickBraggSpot);
    updateStatusBar("--");

    // get coords of click
    double x = ui->fftPlot->xAxis->pixelToCoord(event->pos().x());
    double y = ui->fftPlot->yAxis->pixelToCoord(event->pos().y());
    
    // calculate optimal sigma [REF]
    double sig = minGrad/(4*2.355);

    QColor phaseCol;

    if (phaseSelection == 1)
        phaseCol = Qt::blue;
    else
        phaseCol = Qt::red;

    ui->fftPlot->DrawCircle(x, y, phaseCol, QBrush(phaseCol));
    ui->fftPlot->DrawCircle(x, y, phaseCol, QBrush(Qt::NoBrush), 3*sig); // assume here that 3sigma is the ask radius.

    GPAstrain->calculatePhase(phaseSelection, x, y, sig);

    Eigen::MatrixXd ph = GPAstrain->getPhase(phaseSelection)->getWrappedPhase();

    try
    {
        ui->imagePlot->SetImage(ph);
    }
    catch (const std::exception& e)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error", e.what());
        return;
    }

    auto ret = QMessageBox::information(this, tr("Refine"), tr("Do you want to  refine this g-vector"), QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        selectRefineArea();
    }
    else
    {
        continuePhase();
    }
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
        QMessageBox messageBox;
        messageBox.critical(0,"Error", e.what());
        return;
    }

    if (phaseSelection  == 0)
    { // if this is the first phase, select another.
        ++phaseSelection;
        updateStatusBar("Select another g-vector on the FFT");
        QMessageBox::information(this, tr("GPA"), tr("Select another g-vector on the FFT"), QMessageBox::Ok);
        connect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickBraggSpot);
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
            doRefinement(top, left, bottom, right);
        }
        else
        {
            ui->imagePlot->clearItems();
            ui->imagePlot->replot();
            xCorner.clear();
            yCorner.clear();
            connect(ui->imagePlot, &ImagePlot::mousePress, this, &MainWindow::clickRectCorner);
        }

    }
    else
    {

        xCorner.clear();
        yCorner.clear();
        connect(ui->imagePlot, &ImagePlot::mousePress, this, &MainWindow::clickRectCorner);
    }
}


void MainWindow::clickRectCorner(QMouseEvent *event)
{
    //check vector sies etc for error?

    double x = ui->imagePlot->xAxis->pixelToCoord(event->pos().x());
    double y = ui->imagePlot->yAxis->pixelToCoord(event->pos().y());

    ui->imagePlot->DrawCircle(x, y);

    xCorner.push_back(x);
    yCorner.push_back(y);

    if(xCorner.size() == 2)
    {
        disconnect(ui->imagePlot, &ImagePlot::mousePress, this, &MainWindow::clickRectCorner);

        double top = std::max(yCorner[0], yCorner[1]);
        double bottom = std::min(yCorner[0], yCorner[1]);

        double right = std::max(xCorner[0], xCorner[1]);
        double left = std::min(xCorner[0], xCorner[1]);

        ui->imagePlot->DrawRectangle(top, left, bottom, right);

        auto rect = QMessageBox::information(this, tr("Refine"), tr("Accept area?"), QMessageBox::Yes | QMessageBox::No);

        if (rect == QMessageBox::Yes)
        {
            ui->imagePlot->clearItems();
            ui->imagePlot->replot();
            doRefinement(top, left, bottom, right);
        }
        else
        {
            // clear figure
            ui->imagePlot->clearItems();
            ui->imagePlot->replot();
            xCorner.clear();
            yCorner.clear();
            selectRefineArea();
        }

    }
}


void MainWindow::doRefinement(double top, double left, double bottom, double right)
{
    int rowmid = GPAstrain->getImage()->rows() / 2;
    int colmid = GPAstrain->getImage()->cols() / 2;

    int t = static_cast<int>(top) + rowmid;
    int b = static_cast<int>(bottom) + rowmid;
    int l = static_cast<int>(left) + colmid;
    int r = static_cast<int>(right) + colmid;

    GPAstrain->getPhase(phaseSelection)->refinePhase(t, l, b, r);

    Eigen::MatrixXd ph = GPAstrain->getPhase(phaseSelection)->getWrappedPhase();

    try
    {
        ui->imagePlot->SetImage(ph);
    }
    catch (const std::exception& e)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error", e.what());
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
        connect(ui->imagePlot, &ImagePlot::mousePress, this, &MainWindow::clickRectCorner);
    }
}

void MainWindow::getStrains()
{
    updateStatusBar("GPA completed!");

    ui->tabWidget->setCurrentIndex(1);

    double angle = ui->angleSpin->value();

    GPAstrain->calculateStrain(angle);
    ui->exxPlot->SetImage(*(GPAstrain->getExx()), false);
    ui->exyPlot->SetImage(*(GPAstrain->getExy()), false);
    ui->eyxPlot->SetImage(*(GPAstrain->getEyx()), false);
    ui->eyyPlot->SetImage(*(GPAstrain->getEyy()), false);

    haveStrains = true;

    double lim =  ui->colorBar->GetLimits().upper;

    emit(ui->colorBar->limitsChanged(lim));

    QCPColorGradient map = ui->colorBar->GetColorMap();

    emit(ui->colorBar->mapChanged(map));

    updateOtherPlot(ui->leftCombo->currentIndex(), 0);
    updateOtherPlot(ui->rightCombo->currentIndex(), 1);

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
    }
    if (index == 6 || index== 7)
    {
        auto imSize = GPAstrain->getSize();
        Eigen::MatrixXcd dx(imSize.y, imSize.x);
        Eigen::MatrixXcd dy(imSize.y, imSize.x);
        GPAstrain->getPhase(side)->getDifferential(dx, dy);
        if (index == 6)
            image->SetImage(dx, ShowComplex::Real, rePlot);
        else if (index == 7)
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
    ui->colorBar->SetLimits(lim);
}

void MainWindow::on_colourMapBox_currentIndexChanged(const QString &Map)
{
    ui->colorBar->SetColorMap(Map);
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

    Eigen::MatrixXcd image;

    if (ui->actionHann->isChecked())
        image = UtilsMaths::HannWindow(original_image);
    else
        image = original_image;

    showImageAndFFT(image);
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::ExportAll(int choice)
{
    if(!haveStrains)
        return;

    QSettings settings;

    // get path
    QString fileDir = QFileDialog::getExistingDirectory(this, "Save directory", settings.value("dialog/currentSavePath").toString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (fileDir.isEmpty())
        return;

    settings.setValue("dialog/currentSavePath", fileDir);

    ui->imagePlot->ExportSelector(fileDir, "image", choice);
    ui->fftPlot->ExportSelector(fileDir, "FFT", choice);

    ui->exxPlot->ExportSelector(fileDir, "exx", choice);
    ui->exyPlot->ExportSelector(fileDir, "exy", choice);
    ui->eyxPlot->ExportSelector(fileDir, "eyx", choice);
    ui->eyyPlot->ExportSelector(fileDir, "eyy", choice);

    // need to loop through and show images before exporting them from same plot
    for(int i = 0; i < ui->leftCombo->count(); ++i)
    {
        updateOtherPlot(i, 0, false);
        updateOtherPlot(i, 1, false);

        ui->leftOtherPlot->ExportSelector(fileDir, "Phase1 " + ui->leftCombo->itemText(i), choice);
        ui->rightOtherPlot->ExportSelector(fileDir, "Phase2 " + ui->rightCombo->itemText(i), choice);
    }
    // If I don't clear first it makes the image distorted
    ui->leftOtherPlot->clearImage();
    ui->rightOtherPlot->clearImage();
    updateOtherPlot(ui->leftCombo->currentIndex(), 0);
    updateOtherPlot(ui->rightCombo->currentIndex(), 1);
}

void MainWindow::ExportStrains(int choice)
{
    if(!haveStrains)
        return;

    QSettings settings;

    // get path
    QString fileDir = QFileDialog::getExistingDirectory(this, "Save directory", settings.value("dialog/currentSavePath").toString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (fileDir.isEmpty())
        return;

    settings.setValue("dialog/currentSavePath", fileDir);

    ui->exxPlot->ExportSelector(fileDir, "exx", choice);
    ui->exyPlot->ExportSelector(fileDir, "exy", choice);
    ui->eyxPlot->ExportSelector(fileDir, "eyx", choice);
    ui->eyyPlot->ExportSelector(fileDir, "eyy", choice);
}

void MainWindow::DisconnectAll()
{
    // stop current GPA if in progress
    disconnect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickBraggSpot);
    disconnect(ui->fftPlot, &ImagePlot::mousePress, this, &MainWindow::clickGVector);
    disconnect(ui->imagePlot, &ImagePlot::mousePress, this, &MainWindow::clickRectCorner);
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
    if(!haveStrains)
        return;

    getStrains();
}
