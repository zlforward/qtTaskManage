#include "reportdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QPdfWriter>
#include <QPainter>
#include <QMessageBox>
#include <QDebug>

ReportDialog::ReportDialog(const QList<TaskCard*> &cards, QWidget *parent)
    : QDialog(parent),
      m_statusChartView(nullptr),
      m_priorityChartView(nullptr),
      m_cardsData(cards)
{
    setWindowTitle(QString::fromLocal8Bit("任务报表"));
    setMinimumSize(800, 600);

    // 计算状态和优先级统计
    QMap<TaskCard::Status, int> statusCounts;
    QMap<TaskCard::Priority, int> priorityCounts;
    for (const auto &card : m_cardsData) {
        if (card) { // 确保卡片指针有效
            statusCounts[card->status()]++;
            priorityCounts[card->priority()]++;
        }
    }

    setupUi();
    createStatusChart(statusCounts);
    createPriorityChart(priorityCounts);
}

ReportDialog::~ReportDialog()
{
    // QChartView 会被其父布局自动删除
}

void ReportDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *chartLayout = new QHBoxLayout();

    // 创建状态图表视图
    m_statusChartView = new QChartView();
    m_statusChartView->setRenderHint(QPainter::Antialiasing);
    chartLayout->addWidget(m_statusChartView);

    // 创建优先级图表视图
    m_priorityChartView = new QChartView();
    m_priorityChartView->setRenderHint(QPainter::Antialiasing);
    chartLayout->addWidget(m_priorityChartView);

    mainLayout->addLayout(chartLayout);

    // 添加导出按钮
    QPushButton *exportButton = new QPushButton(QString::fromLocal8Bit("导出为 PDF"), this);
    connect(exportButton, &QPushButton::clicked, this, &ReportDialog::exportToPdf);
    mainLayout->addWidget(exportButton, 0, Qt::AlignCenter);
}

void ReportDialog::createStatusChart(const QMap<TaskCard::Status, int> &statusCounts)
{
    QPieSeries *series = new QPieSeries();
    series->append(QString::fromLocal8Bit("待办"), statusCounts.value(TaskCard::Todo, 0));
    series->append(QString::fromLocal8Bit("进行中"), statusCounts.value(TaskCard::InProgress, 0));
    series->append(QString::fromLocal8Bit("已完成"), statusCounts.value(TaskCard::Done, 0));

    // 使标签可见
    for(auto slice : series->slices()) {
        slice->setLabelVisible();
        slice->setLabel(QString("%1 (%2)").arg(slice->label()).arg(slice->value()));
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString::fromLocal8Bit("任务状态分布 (饼图)"));
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setAnimationOptions(QChart::AllAnimations);

    m_statusChartView->setChart(chart);
}

void ReportDialog::createPriorityChart(const QMap<TaskCard::Priority, int> &priorityCounts)
{
    QBarSet *lowSet = new QBarSet(QString::fromLocal8Bit("低"));
    QBarSet *mediumSet = new QBarSet(QString::fromLocal8Bit("中"));
    QBarSet *highSet = new QBarSet(QString::fromLocal8Bit("高"));

    *lowSet << priorityCounts.value(TaskCard::Low, 0);
    *mediumSet << priorityCounts.value(TaskCard::Medium, 0);
    *highSet << priorityCounts.value(TaskCard::High, 0);

    QBarSeries *series = new QBarSeries();
    series->append(lowSet);
    series->append(mediumSet);
    series->append(highSet);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString::fromLocal8Bit("任务优先级分布 (柱状图)"));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // X 轴 (类别)
    QStringList categories = { QString::fromLocal8Bit("优先级") };
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Y 轴 (值)
    QValueAxis *axisY = new QValueAxis();
    // 计算最大值，确保 Y 轴刻度合适
    int maxCount = 0;
    for (int count : priorityCounts.values()) {
        maxCount = qMax(maxCount, count);
    }
    axisY->setRange(0, qMax(1, maxCount)); // 至少为 1，避免范围为 0
    axisY->setTickCount(qMax(2, maxCount + 1)); // 设置刻度数量
    axisY->setLabelFormat("%d"); // 显示整数
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    m_priorityChartView->setChart(chart);
}

void ReportDialog::saveChartToPdf(QChartView *chartView, const QString &filePath)
{
    if (!chartView || !chartView->chart()) {
        qWarning() << "Chart view or chart is null, cannot save to PDF.";
        QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("无法导出图表：图表视图无效。"));
        return;
    }

    QPdfWriter pdfWriter(filePath);
    pdfWriter.setPageSize(QPagedPaintDevice::A4);
    pdfWriter.setPageMargins(QMargins(30, 30, 30, 30));

    QPainter painter(&pdfWriter);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制图表到 PDF
    chartView->render(&painter);

    painter.end();

    QMessageBox::information(this, QString::fromLocal8Bit("导出成功"), QString::fromLocal8Bit("图表已成功导出为 PDF 文件: ") + filePath);
}

void ReportDialog::exportToPdf()
{
    QString filePath = QFileDialog::getSaveFileName(this, QString::fromLocal8Bit("导出报表为 PDF"), "", "PDF Files (*.pdf)");
    if (filePath.isEmpty()) {
        return;
    }

    QPdfWriter pdfWriter(filePath);
    pdfWriter.setPageSize(QPagedPaintDevice::A4);
    pdfWriter.setPageMargins(QMargins(30, 30, 30, 30));

    QPainter painter(&pdfWriter);
    painter.setRenderHint(QPainter::Antialiasing);

    // 设置绘制区域
    const QRectF pageRect = painter.viewport();
    const qreal titleHeight = 40; // 为标题留出空间
    const qreal chartSpacing = 15; // 图表之间的间距
    const qreal availableHeight = pageRect.height() - titleHeight - chartSpacing;
    const qreal chartHeight = availableHeight / 2.0;

    QRectF statusRect(pageRect.left(), pageRect.top() + titleHeight, pageRect.width(), chartHeight);
    QRectF priorityRect(pageRect.left(), statusRect.bottom() + chartSpacing, pageRect.width(), chartHeight);

    // 绘制标题
    painter.setFont(QFont("Arial", 16, QFont::Bold));
    painter.drawText(QRectF(pageRect.left(), pageRect.top(), pageRect.width(), titleHeight), Qt::AlignCenter, QString::fromLocal8Bit("任务管理报表"));
    painter.setFont(QFont()); // 恢复默认字体

    // 绘制状态图表
    if (m_statusChartView && m_statusChartView->chart()) {
        m_statusChartView->render(&painter, statusRect, m_statusChartView->rect()); // 绘制到指定区域
    } else {
        qWarning() << "Status chart view or chart is null, cannot export.";
        painter.drawText(statusRect, Qt::AlignCenter, QString::fromLocal8Bit("状态图表无法加载"));
    }

    // 绘制优先级图表
    if (m_priorityChartView && m_priorityChartView->chart()) {
        m_priorityChartView->render(&painter, priorityRect, m_priorityChartView->rect()); // 绘制到指定区域
    } else {
        qWarning() << "Priority chart view or chart is null, cannot export.";
        painter.drawText(priorityRect, Qt::AlignCenter, QString::fromLocal8Bit("优先级图表无法加载"));
    }

    painter.end();

    QMessageBox::information(this, QString::fromLocal8Bit("导出成功"), QString::fromLocal8Bit("报表已成功导出为 PDF 文件。"));
} 