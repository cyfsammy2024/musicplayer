#ifndef TAGEDITORWINDOW_H
#define TAGEDITORWINDOW_H

// 标签编辑器对话框。构造时加载指定文件的标签到表单，保存时写回。
// 保存成功后发出 tagsSaved 信号，主窗口据此刷新当前播放曲目的歌词显示。

#include <QDialog>
#include <QString>
#include "tagmanager.h"

class QLineEdit;
class QSpinBox;
class QPlainTextEdit;
class QLabel;

class TagEditorWindow : public QDialog
{
    Q_OBJECT
public:
    explicit TagEditorWindow(const QString &filePath, QWidget *parent = nullptr);

signals:
    // 保存成功后发射，参数为被编辑的文件路径
    void tagsSaved(const QString &filePath);

private slots:
    void onSave();

private:
    QString m_filePath;
    QLabel *m_fileLabel;
    QLineEdit *m_titleEdit;
    QLineEdit *m_artistEdit;
    QLineEdit *m_albumEdit;
    QSpinBox *m_yearSpin;
    QSpinBox *m_trackSpin;
    QLineEdit *m_genreEdit;
    QPlainTextEdit *m_lyricsEdit;

    void setupUI();
    void loadFromDisk();
};

#endif // TAGEDITORWINDOW_H
