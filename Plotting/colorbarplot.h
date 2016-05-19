#ifndef COLORBARPLOT_H
#define COLORBARPLOT_H

#include "qcustomplot.h"

class ColorBarPlot : public QCustomPlot
{
    Q_OBJECT
signals:
    //todo add signals in here to connect to other plots
    void mapChanged(QCPColorGradient map);
    void limitsChanged(double limit);

public:
    ColorBarPlot(QWidget *parent = 0) : QCustomPlot(parent)
    {
        ColorBar = new QCPColorScale(this);

        // remove  the crap we don't want
        plotLayout()->removeAt(0);
        plotLayout()->addElement(0, 0, ColorBar);

        ColorBar->setType(QCPAxis::atRight);
        //ColorBar->axis()->setLabel("Strain");

        CreateColorMaps();
    }

    void SetColorMap(const QString &Map)
    {
        if (Map == "Greyscale")
            ColorBar->setGradient(QCPColorGradient::gpGrayscale);
        else if (Map == "Thermal")
            ColorBar->setGradient(QCPColorGradient::gpThermal);
        else if (Map == "Polar")
            ColorBar->setGradient(QCPColorGradient::gpPolar);
        else if (Map == "BlOr")
            ColorBar->setGradient(BluOr_Map);
        else if (Map == "Jet like")
            ColorBar->setGradient(JetLike_Map);
        replot();
        emit mapChanged(GetColorMap());
    }

    QCPColorGradient GetColorMap()
    {
        return ColorBar->gradient();
    }

    void SetLimits(double limit)
    {
        ColorBar->setDataRange(QCPRange(-limit, limit));

        replot();
        emit limitsChanged(limit);
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

        JetLike_Map.setColorStopAt(0.0, QColor(127, 0, 255));
        JetLike_Map.setColorStopAt(0.15, QColor(76, 79, 251));
        JetLike_Map.setColorStopAt(0.4, QColor(76, 242, 206));
        JetLike_Map.setColorStopAt(0.6, QColor(178, 242, 149));
        JetLike_Map.setColorStopAt(0.85, QColor(255, 79, 40));
        JetLike_Map.setColorStopAt(1.0, QColor(255, 0, 0));
    }

};

#endif // COLORBARPLOT_H
