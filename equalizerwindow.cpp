#include "equalizerwindow.h"
#include <QApplication>

EqualizerWindow::EqualizerWindow(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("均衡器");
    setMinimumSize(600, 300);
    setupUI();
    createFrequencyBands();
    loadPresets();
}

EqualizerWindow::~EqualizerWindow()
{
}

void EqualizerWindow::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // 预设选择
    QHBoxLayout *presetLayout = new QHBoxLayout();
    QLabel *presetLabel = new QLabel("预设:");
    m_presetComboBox = new QComboBox();
    presetLayout->addWidget(presetLabel);
    presetLayout->addWidget(m_presetComboBox);
    m_mainLayout->addLayout(presetLayout);
    
    // 均衡器组
    m_eqGroupBox = new QGroupBox("10段均衡器");
    m_eqLayout = new QVBoxLayout(m_eqGroupBox);
    m_bandLayout = new QHBoxLayout();
    m_eqLayout->addLayout(m_bandLayout);
    m_mainLayout->addWidget(m_eqGroupBox);
    
    connect(m_presetComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &EqualizerWindow::onPresetChanged);
}

void EqualizerWindow::createFrequencyBands()
{
    // 10段均衡器频率
    QStringList frequencies = {"31Hz", "62Hz", "125Hz", "250Hz", "500Hz", "1kHz", "2kHz", "4kHz", "8kHz", "16kHz"};
    
    for (const QString &freq : frequencies) {
        Band band;
        
        QVBoxLayout *bandLayout = new QVBoxLayout();
        
        band.label = new QLabel(freq);
        band.label->setAlignment(Qt::AlignCenter);
        bandLayout->addWidget(band.label);
        
        band.slider = new QSlider(Qt::Vertical);
        band.slider->setRange(-12, 12);
        band.slider->setValue(0);
        band.slider->setTickInterval(2);
        band.slider->setTickPosition(QSlider::TicksBothSides);
        bandLayout->addWidget(band.slider);
        
        QLabel *valueLabel = new QLabel("0");
        valueLabel->setAlignment(Qt::AlignCenter);
        bandLayout->addWidget(valueLabel);
        
        m_bandLayout->addLayout(bandLayout);
        m_bands.append(band);
        
        connect(band.slider, &QSlider::valueChanged, valueLabel, [valueLabel](int value) {
            valueLabel->setText(QString::number(value));
        });
        connect(band.slider, &QSlider::valueChanged, this, &EqualizerWindow::onSliderChanged);
    }
}

void EqualizerWindow::loadPresets()
{
    m_presetNames << "自定义" << "流行" << "摇滚" << "古典" << "爵士" << "电子";
    
    // 预设值，每个频段的值范围为-12到12
    m_presets.append(QList<int>()); // 自定义
    m_presets.append({0, 0, 0, 0, 0, 0, 0, 0, 0, 0}); // 流行
    m_presets.append({6, 4, 2, -2, -4, -2, 0, 2, 4, 6}); // 摇滚
    m_presets.append({-4, -4, -2, 0, 2, 4, 4, 2, 0, -2}); // 古典
    m_presets.append({2, 4, 6, 4, 2, 0, -2, -4, -2, 0}); // 爵士
    m_presets.append({6, 6, 4, 2, 0, 0, 2, 4, 6, 6}); // 电子
    
    m_presetComboBox->addItems(m_presetNames);
}

void EqualizerWindow::onPresetChanged(int index)
{
    if (index > 0 && index < m_presets.size()) {
        applyPreset(m_presets[index]);
    }
}

void EqualizerWindow::applyPreset(const QList<int> &preset)
{
    if (preset.size() == m_bands.size()) {
        for (int i = 0; i < m_bands.size(); ++i) {
            m_bands[i].slider->setValue(preset[i]);
        }
        emitSettingsChanged();
    }
}

void EqualizerWindow::onSliderChanged(int value)
{
    Q_UNUSED(value);
    // 当用户手动调整滑块时，将预设设置为自定义
    m_presetComboBox->setCurrentIndex(0);
    emitSettingsChanged();
}

QList<int> EqualizerWindow::getEqualizerSettings() const
{
    QList<int> settings;
    for (const Band &band : m_bands) {
        settings.append(band.slider->value());
    }
    return settings;
}

void EqualizerWindow::emitSettingsChanged()
{
    emit equalizerSettingsChanged(getEqualizerSettings());
}
