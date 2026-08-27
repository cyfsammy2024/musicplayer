#ifndef LYRICSWINDOW_H
#define LYRICSWINDOW_H

#include <QDialog>
#include <QTextEdit>

class LyricsWindow : public QDialog
{
    Q_OBJECT
public:
    explicit LyricsWindow(QWidget *parent = nullptr);

public slots:
    // 显示已构建好的歌词 HTML（带高亮的当前行视图）
    void setLyricsHtml(const QString &html);
    // 显示纯文本（如"歌词将显示在这里"、初始全部歌词）
    void setLyricsText(const QString &text);

private:
    QTextEdit *m_lyricsView;
};

#endif // LYRICSWINDOW_H
