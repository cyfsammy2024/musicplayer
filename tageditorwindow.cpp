#include "tageditorwindow.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileInfo>
#include <QMessageBox>

TagEditorWindow::TagEditorWindow(const QString &filePath, QWidget *parent)
    : QDialog(parent), m_filePath(filePath)
{
    setWindowTitle(QStringLiteral("标签编辑"));
    setMinimumSize(480, 420);
    setupUI();
    loadFromDisk();
}

void TagEditorWindow::setupUI()
{
    auto *main = new QVBoxLayout(this);

    m_fileLabel = new QLabel(this);
    m_fileLabel->setWordWrap(true);
    m_fileLabel->setStyleSheet("color: gray;");
    main->addWidget(m_fileLabel);

    auto *form = new QFormLayout();
    m_titleEdit = new QLineEdit(this);
    m_artistEdit = new QLineEdit(this);
    m_albumEdit = new QLineEdit(this);
    m_yearSpin = new QSpinBox(this);
    m_yearSpin->setRange(0, 9999);
    m_yearSpin->setSpecialValueText(QStringLiteral("(无)"));
    m_trackSpin = new QSpinBox(this);
    m_trackSpin->setRange(0, 9999);
    m_trackSpin->setSpecialValueText(QStringLiteral("(无)"));
    m_genreEdit = new QLineEdit(this);
    m_lyricsEdit = new QPlainTextEdit(this);
    m_lyricsEdit->setPlaceholderText(QStringLiteral("歌词（内嵌 USLT/©lyr/Vorbis LYRICS）"));

    form->addRow(QStringLiteral("标题:"), m_titleEdit);
    form->addRow(QStringLiteral("艺术家:"), m_artistEdit);
    form->addRow(QStringLiteral("专辑:"), m_albumEdit);
    form->addRow(QStringLiteral("年份:"), m_yearSpin);
    form->addRow(QStringLiteral("音轨号:"), m_trackSpin);
    form->addRow(QStringLiteral("流派:"), m_genreEdit);
    form->addRow(QStringLiteral("歌词:"), m_lyricsEdit);
    main->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &TagEditorWindow::onSave);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    main->addWidget(buttons);
}

void TagEditorWindow::loadFromDisk()
{
    QFileInfo info(m_filePath);
    m_fileLabel->setText(QStringLiteral("文件: %1").arg(info.fileName()));

    TagFields fields;
    if (!TagManager::read(m_filePath, fields)) {
        QMessageBox::warning(this, QStringLiteral("读取失败"),
                             QStringLiteral("无法读取该文件的标签（格式可能不支持）。"));
        return;
    }
    m_titleEdit->setText(fields.title);
    m_artistEdit->setText(fields.artist);
    m_albumEdit->setText(fields.album);
    m_yearSpin->setValue(fields.year);
    m_trackSpin->setValue(fields.track);
    m_genreEdit->setText(fields.genre);
    m_lyricsEdit->setPlainText(fields.lyrics);
}

void TagEditorWindow::onSave()
{
    TagFields in;
    in.title = m_titleEdit->text();
    in.artist = m_artistEdit->text();
    in.album = m_albumEdit->text();
    in.year = m_yearSpin->value();
    in.track = m_trackSpin->value();
    in.genre = m_genreEdit->text();
    in.lyrics = m_lyricsEdit->toPlainText();

    QString errMsg;
    if (!TagManager::write(m_filePath, in, &errMsg)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             errMsg.isEmpty() ? QStringLiteral("写入失败，请检查文件权限或格式。")
                                              : errMsg);
        return;
    }
    QMessageBox::information(this, QStringLiteral("已保存"),
                             QStringLiteral("标签已写入。"));
    emit tagsSaved(m_filePath);
    accept();
}
