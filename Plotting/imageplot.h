#ifndef IMAGEPLOT
#define IMAGEPLOT

#include <complex>
#include <memory>
#include <cmath>

#include "qcustomplot.h"
#include "tiffio.h"

#include "exceptions.h"
#include "matrix.h"

#include <iostream>
#include <fstream>

enum ShowComplex{
        Real,
        Complex,
        Phase,
        Amplitude,
        PowerSpectrum};

class ImagePlot : public QCustomPlot
{
    Q_OBJECT
public:
    ImagePlot(QWidget *parent = 0) : QCustomPlot(parent)
    {
        setInteractions(QCP::iRangeZoom);
        setContextMenuPolicy(Qt::CustomContextMenu);

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

        // make the colours look nice
        QPen axesPen = QPen(Qt::DashLine);
        axesPen.setColor(Qt::white);
        setBackground(QBrush(QColor(230, 230, 230)));
        xAxis->grid()->setPen(axesPen);
        yAxis->grid()->setPen(axesPen);

        axisRect()->setAutoMargins(QCP::msNone);
        axisRect()->setMinimumMargins(QMargins(0,0,0,0));
        axisRect()->setMargins(QMargins(0,0,0,0));

        connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));
    }


    void SetImage(const Matrix<double>& image, bool doReplot = true)
    {
        SetImage(image.getData(), image.cols(), image.rows(), doReplot);
    }

    void SetImage(const Matrix<std::complex<double>>& image, ShowComplex show, bool doReplot = true)
    {
        SetImage(image.getData(), image.cols(), image.rows(), show, doReplot);
    }

    void SetImage(const std::vector<double>& image, const int sx, const int sy, bool doReplot = true)
    {
        if (sx*sy != (int)image.size())
            throw sizeError;

        AspectRatio = (double)sx/(double)sy;

        rescaleAxes();
        setImageRatio();

        ImageObject = new QCPColorMap(xAxis, yAxis);
        addPlottable(ImageObject);
        ImageObject->setGradient(QCPColorGradient::gpGrayscale); // default
        ImageObject->setInterpolate(false);

        //check image is same size as dimensions given
        //check imageobject is not null?
        ImageObject->data()->setSize(sx, sy);
        ImageObject->data()->setRange(QCPRange(-(double)sx/2, (double)sx/2), QCPRange(-(double)sy/2, (double)sy/2));
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

        AspectRatio = (double)sx/(double)sy;

        ImageObject = new QCPColorMap(xAxis, yAxis);
        addPlottable(ImageObject);
        ImageObject->setGradient(QCPColorGradient::gpGrayscale); // default
        ImageObject->setInterpolate(false);

        //check image is same size as dimensions given
        //check imageobject is not null?
        ImageObject->data()->setSize(sx, sy);
        ImageObject->data()->setRange(QCPRange(-(double)sx/2, (double)sx/2), QCPRange(-(double)sy/2, (double)sy/2));
        if (show == ShowComplex::Real)
        {
            for (int xIndex=0; xIndex<sx; ++xIndex)
              for (int yIndex=0; yIndex<sy; ++yIndex)
                  ImageObject->data()->setCell(xIndex, yIndex, std::real(image[yIndex*sx+xIndex]));
        }
        else if (show == ShowComplex::Complex)
        {
            for (int xIndex=0; xIndex<sx; ++xIndex)
              for (int yIndex=0; yIndex<sy; ++yIndex)
                  ImageObject->data()->setCell(xIndex, yIndex, std::imag(image[yIndex*sx+xIndex]));
        }
        else if (show == ShowComplex::Phase)
        {
            for (int xIndex=0; xIndex<sx; ++xIndex)
              for (int yIndex=0; yIndex<sy; ++yIndex)
                  ImageObject->data()->setCell(xIndex, yIndex, std::arg(image[yIndex*sx+xIndex]));
        }
        else if (show == ShowComplex::Amplitude)
        {
            for (int xIndex=0; xIndex<sx; ++xIndex)
              for (int yIndex=0; yIndex<sy; ++yIndex)
                  ImageObject->data()->setCell(xIndex, yIndex, std::abs(image[yIndex*sx+xIndex]));
        }
        else if (show == ShowComplex::PowerSpectrum)
        {
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

    void DrawCircle(double x, double y, QColor colour = Qt::red, QBrush fill = QBrush(Qt::red), double radius = 2, double thickness = 2)
    {
        std::shared_ptr<QCPItemEllipse> circle(new QCPItemEllipse(this));
        circle->setPen(QPen(colour, thickness));
        circle->setBrush(fill);
        circle->topLeft->setCoords(x-radius, y-radius);
        circle->bottomRight->setCoords(x+radius, y+radius);
        Ellipses.push_back(circle);
        addItem(circle.get());
        replot();
    }

    void DrawRectangle(double t, double l, double b, double r, QColor colour = Qt::red, QBrush fill = QBrush(Qt::NoBrush), double thickness = 2)
    {
        std::shared_ptr<QCPItemRect> rect(new QCPItemRect(this));
        rect->setPen(QPen(colour, thickness));
        rect->setBrush(fill);
        rect->topLeft->setCoords(l, t);
        rect->bottomRight->setCoords(r, b);
        Rects.push_back(rect);
        addItem(rect.get());
        replot();
    }

    void clearImage(bool doReplot = true)
    {
        haveImage = false;
        clearPlottables();
        if (doReplot)
            replot();
    }

private:
    QCPColorMap *ImageObject;

    bool haveImage = false;

    double AspectRatio = 1;

    int size_x, size_y;

    std::vector<std::shared_ptr<QCPItemEllipse>> Ellipses;

    std::vector<std::shared_ptr<QCPItemRect>> Rects;

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

        saveMenu->addAction("RGB image", this,  SLOT(ExportImage()));
        saveMenu->addAction("Data image", this,  SLOT(ExportData()));
        saveMenu->addAction("Binary", this,  SLOT(ExportBinary()));

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
        yAxis->setRange(QCPRange(-size_y/2-1, size_y/2+2));
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
       TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);
       TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
       TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
       TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

       // virtually nothing supports 64-bit tiff so we will convert it here.
       std::vector<float> buffer(size_x*size_y);
       double* data_temp = ImageObject->data()->getDataArray();

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

        out.write((char *) ImageObject->data()->getDataArray(), size_x*size_y*sizeof(double));

        out.close();
    }
};

#endif // IMAGEPLOT

