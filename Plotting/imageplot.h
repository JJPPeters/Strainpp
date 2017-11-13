#ifndef IMAGEPLOT
#define IMAGEPLOT

#ifndef EIGEN_DEFAULT_TO_ROW_MAJOR
#define EIGEN_DEFAULT_TO_ROW_MAJOR
#endif

#ifndef EIGEN_INITIALIZE_MATRICES_BY_ZERO
#define EIGEN_INITIALIZE_MATRICES_BY_ZERO
#endif

#include <complex>
#include <memory>
#include <cmath>

#include "qcustomplot.h"
#include "tiffio.h"

#include "exceptions.h"
#include <Eigen/Dense>

#include <iostream>
#include <fstream>

enum ShowComplex{
        Real,
        Complex,
        Phase,
        Amplitude,
        PowerSpectrum};

typedef std::map<std::string, std::map<std::string, std::string>>::iterator it_type;

// this wrapper class just serves to return a data array. I could just store an array, but it is
// stored anyway and it is only needed for saving images.
class QCPDataColorMap : public QCPColorMap
{
public:

    QCPDataColorMap(QCPAxis *keyAxis, QCPAxis *valueAxis) : QCPColorMap(keyAxis, valueAxis) {}

    std::vector<double> getDataArray()
    {
        // key is x, value is y?
        std::vector<double> output(data()->valueSize()*data()->keySize());

        // the odd indexing is because we need to export the image 'upside down'
        #pragma omp parallel for
        for (int i = 0; i < data()->valueSize(); ++i)
            for (int j = 0; j < data()->keySize(); ++j)
                output[ i*data()->keySize() + j ] = data()->cell(j, data()->valueSize()-1-i);

        return output;
    }
};

class ImagePlot : public QCustomPlot
{
    Q_OBJECT

public:
    ImagePlot(QWidget *parent = 0) : QCustomPlot(parent)
    {
        setInteractions(QCP::iRangeZoom);
        setContextMenuPolicy(Qt::CustomContextMenu);
        setFocusPolicy(Qt::StrongFocus);

        // this will remove all the crud but keep the grid lines
        xAxis->setSubTickPen(Qt::NoPen);
        xAxis->setTickPen(Qt::NoPen);
        xAxis->setTickLabels(false);
        xAxis->setBasePen(Qt::NoPen);
        yAxis->setSubTickPen(Qt::NoPen);
        yAxis->setTickPen(Qt::NoPen);
        yAxis->setTickLabels(false);
        yAxis->setBasePen(Qt::NoPen);


        // to get origin in centre
        xAxis->setRange(-500, 500);
        yAxis->setRange(-500, 500);

        matchPlotToPalette();

        axisRect()->setAutoMargins(QCP::msNone);
        axisRect()->setMinimumMargins(QMargins(0,0,0,0));
        axisRect()->setMargins(QMargins(0,0,0,0));

        connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));
    }

    // this is used to update the colours by filtering the events
    bool event(QEvent *event)
    {
        // this might get spammed a bit, not sure if it is supposed to
        if (event->type() == QEvent::PaletteChange)
        {
            matchPlotToPalette();
            replot();
        }

        // very important or no other events will get through
        return QCustomPlot::event(event);
    }

    void matchPlotToPalette()
    {
        QPalette pal = qApp->palette();
        QPen axesPen = QPen(Qt::DashLine);
        axesPen.setColor(pal.color(QPalette::Mid));

        setBackground(qApp->palette().brush(QPalette::Background));
        xAxis->grid()->setPen(axesPen);
        yAxis->grid()->setPen(axesPen);

        QPen zeroPen = QPen(Qt::SolidLine);
        zeroPen.setWidth(2);
        zeroPen.setColor(pal.color(QPalette::Mid));

        xAxis->grid()->setZeroLinePen(zeroPen);
        yAxis->grid()->setZeroLinePen(zeroPen);
    }

    void SetImage(const Eigen::MatrixXd& image, bool doReplot = true)
    {
        std::vector<double> buffer(image.size());
        Eigen::Map<Eigen::MatrixXd>(&buffer[0], image.rows(), image.cols()) = image;
        SetImage(buffer, image.cols(), image.rows(), doReplot);
    }

    void SetImage(const Eigen::MatrixXcd& image, ShowComplex show, bool doReplot = true)
    {
        std::vector<std::complex<double>> buffer(image.size());
        Eigen::Map<Eigen::MatrixXcd>(&buffer[0], image.rows(), image.cols()) = image;

        SetImage(buffer, image.cols(), image.rows(), show, doReplot);
    }

    void SetImage(const std::vector<double>& image, const int sx, const int sy, bool doReplot = true)
    {
        if (sx*sy != (int)image.size())
            throw sizeError;

        clearImage();

        AspectRatio = (double)sx/(double)sy;

        rescaleAxes();
        setImageRatio();

        ImageObject = new QCPDataColorMap(xAxis, yAxis);
        addPlottable(ImageObject);
        ImageObject->setGradient(QCPColorGradient::gpGrayscale); // default
        ImageObject->setInterpolate(false);

        //check image is same size as dimensions given
        //check imageobject is not null?
        ImageObject->data()->setSize(sx, sy);
        ImageObject->data()->setRange(QCPRange(-(double)sx/2, (double)sx/2), QCPRange(-(double)sy/2, (double)sy/2));
        #pragma omp parallel for
        for (int xIndex=0; xIndex<sx; ++xIndex)
          for (int yIndex=0; yIndex<sy; ++yIndex)
            ImageObject->data()->setCell(xIndex, yIndex, image[yIndex*sx+xIndex]);

        size_x = sx;
        size_y = sy;

        ImageObject->rescaleDataRange();
        rescaleAxes();
        setImageRatio();
        if (doReplot)
            replot();

        haveImage = true;
    }


    void SetImage(const std::vector<std::complex<double>>& image, const int sx, const int sy, ShowComplex show, bool doReplot = true)
    {
        if (sx*sy != (int)image.size())
            throw sizeError;

        clearImage();

        AspectRatio = (double)sx/(double)sy;

        rescaleAxes();
        setImageRatio();

        ImageObject = new QCPDataColorMap(xAxis, yAxis);
        addPlottable(ImageObject);
        ImageObject->setGradient(QCPColorGradient::gpGrayscale); // default
        ImageObject->setInterpolate(false);

        //check image is same size as dimensions given
        //check imageobject is not null?
        ImageObject->data()->setSize(sx, sy);
        ImageObject->data()->setRange(QCPRange(-(double)sx/2, (double)sx/2), QCPRange(-(double)sy/2, (double)sy/2));
        if (show == ShowComplex::Real)
        {
            #pragma omp parallel for
            for (int xIndex=0; xIndex<sx; ++xIndex)
              for (int yIndex=0; yIndex<sy; ++yIndex)
                  ImageObject->data()->setCell(xIndex, yIndex, std::real(image[yIndex*sx+xIndex]));
        }
        else if (show == ShowComplex::Complex)
        {
            #pragma omp parallel for
            for (int xIndex=0; xIndex<sx; ++xIndex)
              for (int yIndex=0; yIndex<sy; ++yIndex)
                  ImageObject->data()->setCell(xIndex, yIndex, std::imag(image[yIndex*sx+xIndex]));
        }
        else if (show == ShowComplex::Phase)
        {
            #pragma omp parallel for
            for (int xIndex=0; xIndex<sx; ++xIndex)
              for (int yIndex=0; yIndex<sy; ++yIndex)
                  ImageObject->data()->setCell(xIndex, yIndex, std::arg(image[yIndex*sx+xIndex]));
        }
        else if (show == ShowComplex::Amplitude)
        {
            #pragma omp parallel for
            for (int xIndex=0; xIndex<sx; ++xIndex)
              for (int yIndex=0; yIndex<sy; ++yIndex)
                  ImageObject->data()->setCell(xIndex, yIndex, std::abs(image[yIndex*sx+xIndex]));
        }
        else if (show == ShowComplex::PowerSpectrum)
        {
            #pragma omp parallel for
            for (int xIndex=0; xIndex<sx; ++xIndex)
              for (int yIndex=0; yIndex<sy; ++yIndex)
                  ImageObject->data()->setCell(xIndex, yIndex, std::log10(1+std::abs(image[yIndex*sx+xIndex])));
        }

        size_x = sx;
        size_y = sy;

        ImageObject->rescaleDataRange();
        rescaleAxes();
        setImageRatio();

        if (doReplot)
            replot();

        haveImage = true;
    }

    void DrawCircle(double x, double y, QColor colour = Qt::red, QBrush fill = QBrush(Qt::red), double radius = 2, Qt::PenStyle line = Qt::SolidLine, double thickness = 2)
    {
        QCPItemEllipse* circle(new QCPItemEllipse(this));
        circle->setPen(QPen(colour, thickness, line));
        circle->setBrush(fill);
        circle->topLeft->setCoords(x-radius, y-radius);
        circle->bottomRight->setCoords(x+radius, y+radius);
        addItem(circle);
        replot();
    }

    void DrawRectangle(double t, double l, double b, double r, QColor colour = Qt::red, QBrush fill = QBrush(Qt::NoBrush), double thickness = 2)
    {
        QCPItemRect* rect(new QCPItemRect(this));
        rect->setPen(QPen(colour, thickness));
        rect->setBrush(fill);
        rect->topLeft->setCoords(l, t);
        rect->bottomRight->setCoords(r, b);
        addItem(rect);
        replot();
    }

    void clearImage(bool doReplot = true)
    {
        haveImage = false;
        clearPlottables();
        if (doReplot)
            replot();
    }

    void clearAllItems(bool doReplot = true)
    {
        clearItems();
        if (doReplot)
            replot();
    }

    bool inAxis(double x, double y)
    {
        int bottom = ImageObject->data()->valueRange().lower;
        int top = ImageObject->data()->valueRange().upper;
        int left = ImageObject->data()->keyRange().lower;
        int right = ImageObject->data()->keyRange().upper;

        return x < right && x > left && y < top && y > bottom;
    }

private:
    QCPDataColorMap *ImageObject;

    bool haveImage = false;

    double AspectRatio = 1;

    int size_x, size_y;

    int lastWidth, lastHeight;

    void resizeEvent(QResizeEvent* event)
    {
        mPaintBuffer = QPixmap(event->size());
        setViewport(rect());
        setImageRatio(event->size().width(), event->size().height());
        replot(rpQueued);
    }

    void setImageRatio()
    {
        setImageRatio(axisRect()->width(), axisRect()->height());
    }

    // Basically a reimplementation of setScalRatio() but for both axes
    void setImageRatio(int axisWidth, int axisHeight)
    {
        // this case is needed as, when resizing the graph very fast, the values will switch and the calculation will be wrong.
        // this just sets the ratio to the intermediate point when the axes are equal, before then scaling to the correct values.

        if ( lastWidth <= AspectRatio*lastHeight && axisWidth > AspectRatio*axisHeight ) // plot WAS TALL
            yAxis->setRange(yAxis->range().center(), xAxis->range().size() / AspectRatio, Qt::AlignCenter);
        else if (  lastWidth >= AspectRatio*lastHeight && axisWidth < AspectRatio*axisHeight ) // plot WAS WIDE
            xAxis->setRange(xAxis->range().center(), AspectRatio*yAxis->range().size(), Qt::AlignCenter);

        lastWidth = axisWidth;
        lastHeight = axisHeight;

        if (axisWidth < AspectRatio*axisHeight) // plot is TALL
        {
            double newRange = xAxis->range().size()*(double)axisHeight / (double)axisWidth;
            yAxis->setRange(yAxis->range().center(), newRange, Qt::AlignCenter);
        }
        else if (AspectRatio*axisHeight < axisWidth) // plot is WIDE
        {
            double newRange = yAxis->range().size()*(double)axisWidth / (double)axisHeight;
            xAxis->setRange(xAxis->range().center(), newRange, Qt::AlignCenter);
        }
    }

    //There is always an extra pixel it seems so I rewrote this to compensate
    bool saveRastered(const QString &fileName, int width, int height, double scale, const char *format, int quality = -1)
    {
      QPixmap buffer = toPixmap(width, height+1, scale);
      QPixmap cropped = buffer.copy(0, 0, width, height);
      if (!buffer.isNull())
        return cropped.save(fileName, format, quality);
      else
        return false;
    }

public slots:
    void SetColorLimits(double ul)
    {
        if (!haveImage)
            return;

        QCPRange lims(-ul, ul);
        ImageObject->setDataRange(lims);
        replot();
    }

    void SetColorMap(QCPColorGradient Map)
    {
        if (!haveImage)
            return;
        ImageObject->setGradient(Map);
        replot();
    }

    void ExportSelector(QString directory, QString filename, int choice)
    {
        QString filepath = QDir(directory).filePath(filename);

        if (choice == 0)
        {
            filepath += ".tif";
            ExportImage(filepath, false);
        }
        else if (choice == 1)
        {
            filepath += ".tif";
            ExportData(filepath);
        }
        else if (choice == 2)
        {
            filepath += ".bin";
            ExportBinary(filepath);
        }
    }

    void ResetAxes()
    {
        if(!haveImage)
        {
            xAxis->setRange(-500, 500);
            yAxis->setRange(-500, 500);
        }
        rescaleAxes();
        setImageRatio();
        replot();
    }

private slots:
    void contextMenuRequest(QPoint pos)
    {
        QMenu* menu = new QMenu(this);

        menu->addAction("Reset zoom", this, SLOT(ResetAxes()));

        QMenu* saveMenu = new QMenu("Export...", this);

        QAction* expIm = saveMenu->addAction("RGB image", this,  SLOT(ExportImage()));
        expIm->setEnabled(haveImage);
        QAction* expDat = saveMenu->addAction("Data image", this,  SLOT(ExportData()));
        expDat->setEnabled(haveImage);
        QAction* expBin = saveMenu->addAction("Binary", this,  SLOT(ExportBinary()));
        expBin->setEnabled(haveImage);

        menu->addMenu(saveMenu);

        menu->popup(mapToGlobal(pos));
    }

    void ExportImage()
    {
        QSettings settings;

        // get path
        QString filepath = QFileDialog::getSaveFileName(this, "Save image", settings.value("dialog/currentSavePath").toString(), "TIFF (*.tif)");

        if (filepath.isEmpty())
            return;

        QFileInfo temp_file(filepath);
        settings.setValue("dialog/currentSavePath", temp_file.path());

        ExportImage(filepath);
    }

    void ExportImage(QString filepath, bool doReplot = true)
    {
        std::string format = "TIFF";

        QCPRange xr = xAxis->range();
        QCPRange yr = yAxis->range();

        // the Axes are all screwed but this seems to work
        // needed to export the right area
        xAxis->setRange(QCPRange(-size_x/2, size_x/2));
        yAxis->setRange(QCPRange(-size_y/2-1, size_y/2+1));
        saveRastered(filepath, size_x, size_y, 1.0, format.c_str());

        xAxis->setRange(xr);
        yAxis->setRange(yr);

        if(doReplot)
            replot();
    }

    void ExportData()
    {
        QSettings settings;

        // get path
        QString filepath = QFileDialog::getSaveFileName(this, "Save image", settings.value("dialog/currentSavePath").toString(), "TIFF (*.tif)");

        if (filepath.isEmpty())
            return;

        QFileInfo temp_file(filepath);
        settings.setValue("dialog/currentSavePath", temp_file.path());

        ExportData(filepath);
    }

    void ExportData(QString filepath)
    {
       //TODO: need to check for error on opening
       TIFF* out(TIFFOpen(filepath.toStdString().c_str(), "w"));

       if (!out)
           return;

       TIFFSetField(out, TIFFTAG_IMAGEWIDTH, size_x);
       TIFFSetField(out, TIFFTAG_IMAGELENGTH, size_y);
       TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
       TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, sizeof(float)*8);
       TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
       TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, size_y);
//       TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);
       TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
       TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
       TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

       // virtually nothing supports 64-bit tiff so we will convert it here.
       std::vector<float> buffer(size_x*size_y);
       std::vector<double> data_temp = ImageObject->getDataArray();

       for (int i = 0; i < size_x*size_y; ++i)
           buffer[i] = (float)data_temp[i];

       tsize_t image_s;
       if( (image_s = TIFFWriteEncodedStrip(out, 0, &buffer[0], sizeof(float)*size_x*size_y)) == -1)
            std::cerr << "Unable to write tif file" << std::endl;

       (void)TIFFClose(out);
    }

    void ExportBinary()
    {
        QSettings settings;

        // get path
        QString filepath = QFileDialog::getSaveFileName(this, "Save image", settings.value("dialog/currentSavePath").toString(), "Binary (*.bin)");

        if (filepath.isEmpty())
            return;

        QFileInfo temp_file(filepath);
        settings.setValue("dialog/currentSavePath", temp_file.path());

        ExportBinary(filepath);
    }

    void ExportBinary(QString filepath)
    {      
        std::ofstream out(filepath.toStdString(), std::ios::out | std::ios::binary);
        if(!out)
            std::cerr << "Unable to write binary file" << std::endl;

        auto data = ImageObject->getDataArray();

        out.write(reinterpret_cast<const char*>(&data[0]), data.size()*sizeof(double));

        out.close();
    }
};

#endif // IMAGEPLOT

