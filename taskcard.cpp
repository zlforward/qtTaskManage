#include "taskcard.h"
#include <QPainter>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QGraphicsSceneMouseEvent>
#include <QVBoxLayout>
#include <QFont>
#include <QJsonObject>
#include <QJsonDocument>

TaskCard::TaskCard(const QString &title, const QString &description, 
                  Priority priority, Status status, const QDateTime &deadline, QGraphicsItem *parent)
    : QGraphicsWidget(parent), 
      m_title(title), 
      m_description(description), 
      m_status(status), 
      m_priority(priority),
      m_deadline(deadline),
      m_selected(false),
      m_opacity(0.9)  // 设置透明度为10% (不透明度为90%)
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
}

void TaskCard::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    QRectF rect = boundingRect();
    
    // 创建深蓝色背景，参考图片中的风格
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    
    // 根据优先级和状态调整颜色
    QColor baseColor = getPriorityColor();
    
    gradient.setColorAt(0, baseColor.lighter(120));
    gradient.setColorAt(1, baseColor);
    
    // 设置透明度
    painter->setOpacity(m_opacity);
    
    painter->setBrush(gradient);
    
    // 绘制选中状态边框
    if (isSelected()) {
        QPen pen(QColor(0, 255, 0), 2);
        painter->setPen(pen);
    } else {
        painter->setPen(Qt::NoPen);
    }
    
    // 绘制圆角矩形卡片
    painter->drawRoundedRect(rect, 10, 10);
    
    // 添加白色细边框（与图片中的风格一致）
    painter->setPen(QPen(QColor(255, 255, 255, 150), 1));
    painter->drawRoundedRect(rect, 10, 10);
    
    // 渲染卡片内容
    renderCardContent(painter, rect);
}

void TaskCard::renderCardContent(QPainter *painter, const QRectF &rect)
{
    // 保存当前画笔状态
    painter->save();
    
    // 设置文字颜色为白色（与图片中的风格一致）
    painter->setPen(Qt::white);
    
    // 绘制标题
    QFont titleFont("Arial", 12, QFont::Bold);
    painter->setFont(titleFont);
    QRectF titleRect = QRectF(rect.left() + 10, rect.top() + 10, rect.width() - 20, 20);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_title);
    
    // 绘制描述
    QFont descFont("Arial", 9);
    painter->setFont(descFont);
    QRectF descRect = QRectF(rect.left() + 10, rect.top() + 35, rect.width() - 20, 40);
    painter->drawText(descRect, Qt::AlignLeft | Qt::TextWordWrap, m_description);
    
    // 绘制状态指示器
    QString statusText;
    switch (m_status) {
        case Todo: statusText = QString::fromLocal8Bit("待办"); break;
        case InProgress: statusText = QString::fromLocal8Bit("进行中"); break;
        case Done: statusText = QString::fromLocal8Bit("已完成"); break;
    }
    
    QFont statusFont("Arial", 8);
    painter->setFont(statusFont);
    QRectF statusRect = QRectF(rect.left() + 10, rect.bottom() - 25, 60, 20);
    painter->drawText(statusRect, Qt::AlignLeft | Qt::AlignVCenter, statusText);
    
    // 如果有截止日期，则显示
    if (m_deadline.isValid()) {
        QString dateText = m_deadline.toString("MM-dd");
        QRectF dateRect = QRectF(rect.right() - 70, rect.bottom() - 25, 60, 20);
        painter->drawText(dateRect, Qt::AlignRight | Qt::AlignVCenter, dateText);
    }
    
    // 恢复画笔状态
    painter->restore();
}

QColor TaskCard::getPriorityColor() const
{
    switch (m_priority) {
        case Low:
            return QColor(52, 152, 219); // 蓝色
        case Medium:
            return QColor(41, 128, 185); // 深蓝色
        case High:
            return QColor(24, 110, 165); // 更深的蓝色
        default:
            return QColor(41, 128, 185); // 默认深蓝色
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
    
    if (m_deadline.isValid()) {
        json["deadline"] = m_deadline.toString(Qt::ISODate);
    }
    
    QJsonDocument doc(json);
    return QString(doc.toJson(QJsonDocument::Compact));
}