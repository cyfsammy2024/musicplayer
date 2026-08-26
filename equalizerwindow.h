#ifndef EQUALIZERWINDOW_H
#define EQUALIZERWINDOW_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>

class EqualizerWindow : public QDialog
{
    Q_OBJECT

public:
    explicit EqualizerWindow(QWidget *parent = nullptr);
    ~EqualizerWindow();
    
    QList<int> getEqualizerSettings() const;

signals:
    void equalizerSettingsChanged(const QList<int> &settings);

private slots:
    void onPresetChanged(int index);
    void onSliderChanged(int value);

private:
    void setupUI();
    void createFrequencyBands();
    void loadPresets();
    void applyPreset(const QList<int> &preset);
    void emitSettingsChanged();

    QVBoxLayout *m_mainLayout;
    QGroupBox *m_eqGroupBox;
    QVBoxLayout *m_eqLayout;
    QHBoxLayout *m_bandLayout;
    QComboBox *m_presetComboBox;
    
    struct Band {
        QSlider *slider;
        QLabel *label;
    };
    QList<Band> m_bands;
    
    QList<QString> m_presetNames;
    QList<QList<int>> m_presets;
};

#endif // EQUALIZERWINDOW_H
