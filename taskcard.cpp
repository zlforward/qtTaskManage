#include "taskcard.h"
#include <QPainter>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QGraphicsSceneMouseEvent>
#include <QVBoxLayout>
#include <QFont>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>

TaskCard::TaskCard(const QString &title, const QString &description, 
                  Priority priority, Status status, const QDateTime &deadline, 
                  const QString &assignee,
                  QGraphicsItem *parent)
    : QGraphicsWidget(parent), 
      m_title(title), 
      m_description(description), 
      m_status(status), 
      m_priority(priority),
      m_deadline(deadline),
      m_assignee(assignee),
      m_selected(false),
      m_opacity(0.9),
      m_glowIntensity(0.0),
      m_glowIncreasing(true)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptHoverEvents(true);

    // 设置卡片大小与图片中展示的类似
    setMinimumSize(200, 120);
    setMaximumSize(200, 120);
    
    // 使用垂直布局
    m_layout = new QGraphicsLinearLayout(Qt::Vertical, this);
    m_layout->setContentsMargins(10, 10, 10, 10);
    m_layout->setSpacing(5);
    setLayout(m_layout);
    
    // 初始化发光效果计时器
    m_glowTimer = new QTimer(this);
    m_glowTimer->setInterval(50);  // 50ms更新一次
    connect(m_glowTimer, &QTimer::timeout, this, &TaskCard::updateGlowEffect);
    m_glowTimer->start();
}

void TaskCard::updateGlowEffect()
{
    if (m_glowIncreasing) {
        m_glowIntensity += 0.1;
        if (m_glowIntensity >= 1.0) {
            m_glowIntensity = 1.0;
            m_glowIncreasing = false;
        }
    } else {
        m_glowIntensity -= 0.1;
        if (m_glowIntensity <= 0.0) {
            m_glowIntensity = 0.0;
            m_glowIncreasing = true;
        }
    }
    update();  // 触发重绘
}

void TaskCard::drawGlowingText(QPainter *painter, const QRectF &rect, const QString &text, 
                             const QFont &font, const QColor &color, Qt::Alignment alignment)
{
    painter->save();
    painter->setFont(font);
    
    // 绘制发光效果
    for (int i = 3; i >= 0; --i) {
        QColor glowColor = color;
        glowColor.setAlpha(int(50 * m_glowIntensity));  // 发光强度随m_glowIntensity变化
        painter->setPen(QPen(glowColor, i * 2));
        
        // 计算偏移量，确保发光效果居中
        qreal offset = i * 0.5;
        QRectF adjustedRect = rect.adjusted(-offset, -offset, offset, offset);
        
        if (alignment & Qt::TextWordWrap) {
            painter->drawText(adjustedRect, alignment | Qt::TextWordWrap, text);
        } else {
            painter->drawText(adjustedRect, alignment, text);
        }
    }
    
    // 绘制主文本，使用原始rect
    painter->setPen(color);
    if (alignment & Qt::TextWordWrap) {
        painter->drawText(rect, alignment | Qt::TextWordWrap, text);
    } else {
        painter->drawText(rect, alignment, text);
    }
    
    painter->restore();
}

void TaskCard::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    QRectF rect = boundingRect();
    
    // 创建渐变背景
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    
    // 根据优先级和状态调整颜色
    QColor baseColor = getPriorityColor();
    
    // 调整渐变效果
    gradient.setColorAt(0, baseColor.lighter(110));  // 顶部稍微亮一点
    gradient.setColorAt(1, baseColor);               // 底部保持原色
    
    // 设置透明度
    painter->setOpacity(m_opacity);
    
    painter->setBrush(gradient);
    
    // 绘制选中状态边框
    if (isSelected()) {
        QPen pen(QColor(100, 200, 255), 2);  // 更柔和的选中边框颜色
        painter->setPen(pen);
    } else {
        painter->setPen(Qt::NoPen);
    }
    
    // 绘制圆角矩形卡片
    painter->drawRoundedRect(rect, 10, 10);
    
    // 添加白色细边框
    painter->setPen(QPen(QColor(255, 255, 255, 100), 1));  // 更淡的边框
    painter->drawRoundedRect(rect, 10, 10);
    
    // 渲染卡片内容
    renderCardContent(painter, rect);
}

void TaskCard::renderCardContent(QPainter *painter, const QRectF &rect)
{
    painter->save();
    
    // 绘制标题（发光效果）- 加粗
    QFont titleFont("微软雅黑", 12, QFont::Bold);
    QRectF titleRect = QRectF(rect.left() + 10, rect.top() + 10, rect.width() - 20, 20);
    drawGlowingText(painter, titleRect, m_title, titleFont, QColor(220, 220, 220), Qt::AlignLeft | Qt::AlignVCenter);
    
    // 绘制描述（发光效果）- 加粗
    QFont descFont("微软雅黑", 9, QFont::Bold);  // 添加 Bold
    painter->setFont(descFont);
    
    // 计算描述文本区域（从标题下方到状态栏上方的空间）
    QRectF descRect = QRectF(rect.left() + 10, rect.top() + 35, rect.width() - 20, rect.height() - 70);
    
    // 使用 drawGlowingText 绘制自动换行的文本
    QString elidedDesc = painter->fontMetrics().elidedText(m_description, Qt::ElideRight, descRect.width() * 3);
    drawGlowingText(painter, descRect, elidedDesc, descFont, QColor(200, 200, 200), 
                   static_cast<Qt::Alignment>(Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap));
    
    // 绘制状态指示器 - 加粗
    QString statusText;
    switch (m_status) {
        case Todo: statusText = QString::fromLocal8Bit("待办"); break;
        case InProgress: statusText = QString::fromLocal8Bit("进行中"); break;
        case Done: statusText = QString::fromLocal8Bit("已完成"); break;
    }
    
    QFont statusFont("微软雅黑", 8, QFont::Bold);  // 添加 Bold
    qreal statusWidth = m_assignee.isEmpty() ? 60 : 40;
    QRectF statusRect = QRectF(rect.left() + 10, rect.bottom() - 25, statusWidth, 20);
    drawGlowingText(painter, statusRect, statusText, statusFont, QColor(180, 180, 180), Qt::AlignLeft | Qt::AlignVCenter);

    // 绘制执行人（发光效果）- 已经是粗体，保持不变
    if (!m_assignee.isEmpty()) {
        QFont assigneeFont("微软雅黑", 8, QFont::Bold);
        QRectF assigneeRect = QRectF(statusRect.right() + 5, rect.bottom() - 25, 50, 20);
        drawGlowingText(painter, assigneeRect, m_assignee, assigneeFont, QColor(200, 200, 200), 
                       static_cast<Qt::Alignment>(Qt::AlignLeft | Qt::AlignVCenter));
    }

    // 绘制截止日期（发光效果）- 加粗
    if (m_deadline.isValid()) {
        QString dateText = m_deadline.toString("MM-dd");
        qreal dateX = m_assignee.isEmpty() ? rect.right() - 70 : rect.right() - 60;
        QRectF dateRect = QRectF(dateX, rect.bottom() - 25, 50, 20);
        drawGlowingText(painter, dateRect, dateText, statusFont, QColor(180, 180, 180), 
                       static_cast<Qt::Alignment>(Qt::AlignRight | Qt::AlignVCenter));
    }
    
    painter->restore();
}

QColor TaskCard::getPriorityColor() const
{
    switch (m_priority) {
        case Low:
            return QColor(41, 128, 185);  // 浅蓝色
        case Medium:
            return QColor(24, 110, 165);  // 中蓝色
        case High:
            return QColor(15, 89, 145);   // 深蓝色
        default:
            return QColor(24, 110, 165);  // 默认中蓝色
    }
}

void TaskCard::setTitle(const QString &title) 
{ 
    m_title = title; 
    update();
}

QString TaskCard::title() const 
{ 
    return m_title; 
}

void TaskCard::setDescription(const QString &description) 
{ 
    m_description = description; 
    update();
}

QString TaskCard::description() const 
{ 
    return m_description; 
}

void TaskCard::setPriority(Priority priority)
{
    m_priority = priority;
    update();
}

TaskCard::Priority TaskCard::priority() const
{
    return m_priority;
}

void TaskCard::setDeadline(const QDateTime &deadline)
{
    m_deadline = deadline;
    update();
}

QDateTime TaskCard::deadline() const
{
    return m_deadline;
}

void TaskCard::setAssignee(const QString &assignee)
{
    m_assignee = assignee;
    update();
}

QString TaskCard::assignee() const
{
    return m_assignee;
}

void TaskCard::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = pos();
        m_selected = true;
        update(); // 触发重绘以显示选中状态
    }
    QGraphicsWidget::mousePressEvent(event);
}

void TaskCard::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPointF newPos = mapToParent(event->pos() - event->buttonDownPos(Qt::LeftButton));
        setPos(newPos);
    }
    QGraphicsWidget::mouseMoveEvent(event);
}

void TaskCard::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 鼠标释放时，发送信号通知MainWindow更新卡片状态
        emit cardReleased(this);
    }
    QGraphicsWidget::mouseReleaseEvent(event);
}

void TaskCard::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    // 双击事件，发射信号通知MainWindow显示编辑对话框
    emit cardDoubleClicked(this);
    QGraphicsWidget::mouseDoubleClickEvent(event);
}

bool TaskCard::isSelected() const
{
    return m_selected;
}

void TaskCard::setStatus(Status status)
{
    m_status = status;
    update();
}

TaskCard::Status TaskCard::status() const
{
    return m_status;
}

QString TaskCard::toJson() const
{
    QJsonObject json;
    json["title"] = m_title;
    json["description"] = m_description;
    json["status"] = static_cast<int>(m_status);
    json["priority"] = static_cast<int>(m_priority);
    json["assignee"] = m_assignee;
    
    if (m_deadline.isValid()) {
        json["deadline"] = m_deadline.toString(Qt::ISODate);
    }
    
    QJsonDocument doc(json);
    return QString(doc.toJson(QJsonDocument::Compact));
}