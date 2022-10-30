#ifndef COLORBARPLOT_H
#define COLORBARPLOT_H

#include "qcustomplot.h"

class ColorBarPlot : public QCustomPlot
{
    Q_OBJECT
signals:
    //todo add signals in here to connect to other plots
    void mapChanged(QCPColorGradient map, bool rePlot);
    void limitsChanged(double limit, bool rePlot);

public:
    ColorBarPlot(QWidget *parent = 0) : QCustomPlot(parent)
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        setFocusPolicy(Qt::StrongFocus);

        ColorBar = new QCPColorScale(this);

        // remove  the crap we don't want
        plotLayout()->removeAt(0);
        plotLayout()->addElement(0, 0, ColorBar);

        ColorBar->setType(QCPAxis::atRight);
        //ColorBar->axis()->setLabel("Strain");

        CreateColorMaps();

        connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));
    }

    void SetColorMap(const QString &Map, bool rePlot)
    {
        if (Map == "Greyscale")
            ColorBar->setGradient(QCPColorGradient::gpGrayscale);
        else if (Map == "Thermal")
            ColorBar->setGradient(QCPColorGradient::gpThermal);
        else if (Map == "Polar")
            ColorBar->setGradient(QCPColorGradient::gpPolar);
        else if (Map == "BlOr")
            ColorBar->setGradient(BluOr_Map);
        else if (Map == "Turbo")
            ColorBar->setGradient(JetLike_Map);

        if (rePlot)
            replot();

        emit mapChanged(GetColorMap(), rePlot);
    }

    QCPColorGradient GetColorMap()
    {
        return ColorBar->gradient();
    }

    void SetLimits(double limit, bool rePlot)
    {
        ColorBar->setDataRange(QCPRange(-limit, limit));

        if (rePlot)
            replot();
        emit limitsChanged(limit, rePlot);
    }

    QCPRange GetLimits()
    {
        return ColorBar->dataRange();
    }

private:
    QCPColorScale* ColorBar;

    QCPColorGradient BluOr_Map, JetLike_Map;
    QCPColorGradient currentMap;

    void CreateColorMaps()
    {
        // need to remove default stops? (seems to default to cold?)
        BluOr_Map.clearColorStops();

        BluOr_Map.setColorStopAt(0.0, QColor(7, 90, 254));
        BluOr_Map.setColorStopAt(0.5, QColor(235, 255, 235));
        BluOr_Map.setColorStopAt(1.0, QColor(255, 85, 0));

        JetLike_Map.clearColorStops();

//        JetLike_Map.setColorStopAt(0.0, QColor(127, 0, 255));
//        JetLike_Map.setColorStopAt(0.1, QColor(76, 79, 251));
//        JetLike_Map.setColorStopAt(0.2, QColor(25, 150, 242));
//        // JetLike_Map.setColorStopAt(0.325, QColor(24, 205, 227));
//        JetLike_Map.setColorStopAt(0.3, QColor(76, 242, 206));
//        JetLike_Map.setColorStopAt(0.5, QColor(127, 254, 179));
//        JetLike_Map.setColorStopAt(0.7, QColor(178, 242, 149));
//        // JetLike_Map.setColorStopAt(0.675, QColor(230, 205, 115));
//        JetLike_Map.setColorStopAt(0.8, QColor(255, 150, 78));
//        JetLike_Map.setColorStopAt(0.9, QColor(255, 79, 40));
//        JetLike_Map.setColorStopAt(1.0, QColor(255, 0, 0));

        JetLike_Map.setColorStopAt(0.0, QColor(51, 27, 61));
        JetLike_Map.setColorStopAt(0.125, QColor(77, 110, 223));
        JetLike_Map.setColorStopAt(0.25, QColor(61, 185, 233));
        JetLike_Map.setColorStopAt(0.375, QColor(68, 238, 154));
        JetLike_Map.setColorStopAt(0.5, QColor(164, 250, 80));
        JetLike_Map.setColorStopAt(0.625, QColor(235, 206, 76));
        JetLike_Map.setColorStopAt(0.75, QColor(247, 129, 55));
        JetLike_Map.setColorStopAt(0.875, QColor(206, 58, 32));
        JetLike_Map.setColorStopAt(1.0, QColor(119, 21, 19));
    }

private slots:
    void contextMenuRequest(QPoint pos)
    {
        QMenu* menu = new QMenu(this);

        menu->addAction("Export RGB", this, SLOT(ExportImage()));

        menu->popup(mapToGlobal(pos));
    }

public slots:
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

    void ExportImage(QString directory, QString filename)
    {
        QString filepath = QDir(directory).filePath(filename);
        filepath += ".tif";

        ExportImage(filepath);
    }

    void ExportImage(QString filepath)
    {
        int w = width();

        std::string format = "TIFF";
        saveRastered(filepath, w, 512, 1.0, format.c_str());
    }

};

#endif // COLORBARPLOT_H
