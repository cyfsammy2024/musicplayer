#include "equalizerwindow.h"
#include <QApplication>
#include <QSettings>

EqualizerWindow::EqualizerWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("均衡器");
    setMinimumSize(600, 300);
    setupUI();
    createFrequencyBands();
    loadPresets();
    loadSettings();
}

EqualizerWindow::~EqualizerWindow()
{
    saveSettings();
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
        // 阻塞信号：避免 setValue 触发 onSliderChanged 把预设重置为自定义
        for (int i = 0; i < m_bands.size(); ++i) {
            m_bands[i].slider->blockSignals(true);
        }
        for (int i = 0; i < m_bands.size(); ++i) {
            m_bands[i].slider->setValue(preset[i]);
        }
        for (int i = 0; i < m_bands.size(); ++i) {
            m_bands[i].slider->blockSignals(false);
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

void EqualizerWindow::setEqualizerSettings(const QList<int> &settings)
{
    if (settings.size() != m_bands.size()) return;
    for (int i = 0; i < m_bands.size(); ++i) {
        m_bands[i].slider->setValue(settings[i]);
    }
}

void EqualizerWindow::saveSettings() const
{
    QSettings settings("MusicPlayer", "MusicPlayer");
    settings.beginWriteArray("equalizer");
    for (int i = 0; i < m_bands.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("value", m_bands[i].slider->value());
    }
    settings.endArray();
    settings.setValue("equalizerPreset", m_presetComboBox->currentIndex());
}

void EqualizerWindow::loadSettings()
{
    QSettings settings("MusicPlayer", "MusicPlayer");
    int size = settings.beginReadArray("equalizer");
    if (size > 0) {
        QList<int> values;
        for (int i = 0; i < size && i < m_bands.size(); ++i) {
            settings.setArrayIndex(i);
            values.append(settings.value("value", 0).toInt());
        }
        // 先设置预设为自定义，避免 onSliderChanged 期间干扰
        m_presetComboBox->blockSignals(true);
        m_presetComboBox->setCurrentIndex(0);
        m_presetComboBox->blockSignals(false);
        for (int i = 0; i < values.size(); ++i) {
            m_bands[i].slider->setValue(values[i]);
        }
    }
    settings.endArray();
    int preset = settings.value("equalizerPreset", 0).toInt();
    if (preset > 0 && preset < m_presets.size()) {
        m_presetComboBox->blockSignals(true);
        m_presetComboBox->setCurrentIndex(preset);
        m_presetComboBox->blockSignals(false);
        applyPreset(m_presets[preset]);
    }
}
