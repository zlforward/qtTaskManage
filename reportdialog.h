#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

#include <QDialog>
#include <QMap>
#include <QChart>
#include <QChartView>
#include <QPieSeries>
#include <QBarSeries>
#include <QBarSet>
#include <QStringList>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include "taskcard.h"

QT_CHARTS_USE_NAMESPACE

class ReportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReportDialog(const QList<TaskCard*> &cards, QWidget *parent = nullptr);
    ~ReportDialog();

private slots:
    void exportToPdf();

private:
    void setupUi();
    void createStatusChart(const QMap<TaskCard::Status, int> &statusCounts);
    void createPriorityChart(const QMap<TaskCard::Priority, int> &priorityCounts);
    void saveChartToPdf(QChartView *chartView, const QString &filePath);

    QChartView *m_statusChartView;
    QChartView *m_priorityChartView;
    const QList<TaskCard*> &m_cardsData; // 存储任务卡片数据的引用
};

#endif // REPORTDIALOG_H 