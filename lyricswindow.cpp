#include "lyricswindow.h"
#include <QVBoxLayout>

LyricsWindow::LyricsWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("歌词");
    resize(500, 600);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_lyricsView = new QTextEdit(this);
    m_lyricsView->setReadOnly(true);
    m_lyricsView->setLineWrapMode(QTextEdit::WidgetWidth);
    m_lyricsView->setText("歌词将显示在这里");

    layout->addWidget(m_lyricsView);
}

void LyricsWindow::setLyricsHtml(const QString &html)
{
    m_lyricsView->setHtml(html);
}

void LyricsWindow::setLyricsText(const QString &text)
{
    m_lyricsView->setText(text);
}
