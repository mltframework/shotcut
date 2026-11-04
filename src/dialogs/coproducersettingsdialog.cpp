/*
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "coproducersettingsdialog.h"

#include "coproducer/localllamaserverprovider.h"
#include "dialogs/filedownloaddialog.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"
#include "util.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCursor>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int kRefreshTimeoutMs = 5000;
constexpr int kTestTimeoutMs = 30000;

// Paints the model id on the left and its creation date (stored in
// Qt::UserRole + 1) right-aligned, so dates line up like a column.
class ModelListDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const auto date = index.data(Qt::UserRole + 1).toString();
        if (date.isEmpty()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        const auto id = opt.text;
        opt.text.clear();
        auto *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        const QRect rect = opt.rect.adjusted(4, 0, -4, 0);
        painter->save();
        painter->setFont(opt.font);
        painter->setPen(opt.palette.color(
            opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text));
        const QFontMetrics metrics(opt.font);
        const auto dateWidth = metrics.horizontalAdvance(date);
        const auto elidedId = metrics.elidedText(id, Qt::ElideRight, rect.width() - dateWidth - 12);
        painter->drawText(rect, Qt::AlignVCenter | Qt::AlignLeft, elidedId);
        painter->drawText(rect, Qt::AlignVCenter | Qt::AlignRight, date);
        painter->restore();
    }
};

} // namespace

CoproducerSettingsDialog::CoproducerSettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_providerCombo(new QComboBox(this))
    , m_modelEdit(new QComboBox(this))
    , m_baseUrlEdit(new QComboBox(this))
    , m_apiKeyEdit(new QLineEdit(this))
    , m_localModelLabel(new QLineEdit(this))
    , m_localConfigWidget(new QWidget(this))
    , m_modelTable(new QTreeView(this))
    , m_network(new QNetworkAccessManager(this))
{
    setWindowTitle(tr("Coproducer Settings"));

    m_providerCombo->addItem(tr("OpenAI API"), QStringLiteral("openai"));
#ifdef SHOTCUT_HAS_LLAMA_SERVER
    m_providerCombo->addItem(tr("Local model"), QStringLiteral("llama"));
#endif
    connect(m_providerCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &CoproducerSettingsDialog::onProviderChanged);

    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("sk-..."));

    auto index = m_providerCombo->findData(Settings.coproducerProvider());
    m_providerCombo->setCurrentIndex(index >= 0 ? index : 0);
    m_baseUrlEdit->setEditable(true);
    m_baseUrlEdit->addItem(QStringLiteral("https://api.openai.com/v1"));
    m_baseUrlEdit->addItem(QStringLiteral("https://openrouter.ai/api/v1"));
    m_baseUrlEdit->addItem(QStringLiteral("https://api.groq.com/openai/v1"));
    m_baseUrlEdit->setCurrentText(Settings.coproducerBaseUrl());
    m_modelEdit->setEditable(true);
    m_modelEdit->setItemDelegate(new ModelListDelegate(m_modelEdit));
    m_modelEdit->setMinimumContentsLength(40);
    m_modelEdit->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    if (!Settings.coproducerModel().isEmpty())
        m_modelEdit->addItem(Settings.coproducerModel());
    m_modelEdit->setCurrentText(Settings.coproducerModel());
    m_modelEdit->lineEdit()->setPlaceholderText(tr("e.g. gpt-4o-mini"));
    m_apiKeyEdit->setText(Settings.coproducerApiKey());

    // Re-fetch the model list using whatever base URL/key are currently
    // entered (not just what was saved before this dialog was opened),
    // as soon as either one is changed, rather than only on demand.
    connect(m_baseUrlEdit, &QComboBox::activated, this, [this](int) {
        maybeAutoRefreshOpenAiModels();
    });
    connect(m_baseUrlEdit->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
        maybeAutoRefreshOpenAiModels();
    });
    connect(m_apiKeyEdit, &QLineEdit::editingFinished, this, [this]() {
        maybeAutoRefreshOpenAiModels();
    });
    m_localModelLabel->setReadOnly(true);
    m_localModelLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_localModelLabel->setPlaceholderText(tr("Select a downloaded model or browse to choose one"));
    setLocalModelPath(Settings.coproducerModelPath());

    m_model.load(QmlExtension::LLAMA_ID);
    m_modelTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_modelTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_modelTable->setItemsExpandable(false);
    m_modelTable->setRootIsDecorated(false);
    m_modelTable->setUniformRowHeights(true);
    m_modelTable->setModel(&m_model);
    m_modelTable->setWordWrap(false);
    updateModelStatus();
    m_modelTable->header()->setStretchLastSection(false);
    m_modelTable->header()->setSectionResizeMode(ExtensionModel::COLUMN_STATUS, QHeaderView::Fixed);
    m_modelTable->setColumnWidth(ExtensionModel::COLUMN_STATUS, fontMetrics().height());
    m_modelTable->header()->setSectionResizeMode(ExtensionModel::COLUMN_NAME, QHeaderView::Stretch);
    m_modelTable->header()->setSectionResizeMode(ExtensionModel::COLUMN_SIZE, QHeaderView::Fixed);
    m_modelTable->setColumnWidth(ExtensionModel::COLUMN_SIZE,
                                 fontMetrics().horizontalAdvance("XXX.XX XXX") + 12);
    // Show several rows without growing the whole dialog to fit the full
    // (now much longer) model list; the rest is reachable by scrolling.
    m_modelTable->setMinimumHeight(m_modelTable->header()->sizeHint().height()
                                   + (fontMetrics().height() + 8) * 7);
    connect(m_modelTable,
            &QAbstractItemView::clicked,
            this,
            &CoproducerSettingsDialog::onModelRowClicked);
    m_modelTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_modelTable,
            &QWidget::customContextMenuRequested,
            this,
            &CoproducerSettingsDialog::onModelContextMenu);

    auto localForm = new QFormLayout;
    auto localModelRow = new QHBoxLayout;
    localModelRow->addWidget(m_localModelLabel, /*stretch*/ 1);
    auto browseButton = new QToolButton(this);
    browseButton->setIcon(
        QIcon::fromTheme("document-open", QIcon(":/icons/oxygen/32x32/actions/document-open.png")));
    browseButton->setToolTip(tr("Browse..."));
    connect(browseButton,
            &QToolButton::clicked,
            this,
            &CoproducerSettingsDialog::onBrowseModelClicked);
    localModelRow->addWidget(browseButton, /*stretch*/ 0);
    auto refreshButton = new QToolButton(this);
    refreshButton->setIcon(
        QIcon::fromTheme("view-refresh", QIcon(":/icons/oxygen/32x32/actions/view-refresh.png")));
    refreshButton->setToolTip(tr("Refresh Models"));
    connect(refreshButton,
            &QToolButton::clicked,
            this,
            &CoproducerSettingsDialog::onRefreshModelsClicked);
    localModelRow->addWidget(refreshButton, /*stretch*/ 0);
    localForm->addRow(tr("Model:"), localModelRow);

    auto localLayout = new QVBoxLayout(m_localConfigWidget);
    localLayout->addLayout(localForm);
    localLayout->addWidget(m_modelTable);

    auto form = new QFormLayout;
    form->addRow(tr("Provider:"), m_providerCombo);
    m_modelLabel = new QLabel(tr("Model:"), this);
    m_apiKeyLabel = new QLabel(tr("API Key:"), this);
    m_baseUrlLabel = new QLabel(tr("Base URL:"), this);
    form->addRow(m_baseUrlLabel, m_baseUrlEdit);
    auto modelRow = new QHBoxLayout;
    modelRow->addWidget(m_modelEdit, /*stretch*/ 1);
    m_openAiRefreshButton = new QToolButton(this);
    m_openAiRefreshButton->setIcon(
        QIcon::fromTheme("view-refresh", QIcon(":/icons/oxygen/32x32/actions/view-refresh.png")));
    m_openAiRefreshButton->setToolTip(tr("Refresh Models"));
    connect(m_openAiRefreshButton,
            &QToolButton::clicked,
            this,
            &CoproducerSettingsDialog::onRefreshOpenAiModelsClicked);
    modelRow->addWidget(m_openAiRefreshButton, /*stretch*/ 0);
    form->addRow(m_modelLabel, modelRow);
    form->addRow(m_apiKeyLabel, m_apiKeyEdit);

    const auto apiKeyNote = tr("The API key is sent directly to the provider and stored locally "
                               "in your Shotcut settings.");
    m_apiKeyLabel->setToolTip(apiKeyNote);
    m_apiKeyEdit->setToolTip(apiKeyNote);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox = buttons;
    connect(buttons, &QDialogButtonBox::accepted, this, &CoproducerSettingsDialog::onAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_testButton = new QPushButton(tr("Test"), this);
    m_testButton->setToolTip(tr("Send a short test chat message to check the configuration"));
    connect(m_testButton, &QPushButton::clicked, this, &CoproducerSettingsDialog::onTestClicked);
    m_testStatusLabel = new QLabel(this);

    auto layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_localConfigWidget);
    auto buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_testButton, /*stretch*/ 0);
    buttonRow->addWidget(m_testStatusLabel, /*stretch*/ 1);
    buttonRow->addWidget(buttons, /*stretch*/ 0);
    layout->addLayout(buttonRow);

    // Size for the larger of the two providers' sections up front (with both
    // temporarily shown) so switching providers later doesn't resize the
    // dialog -- only the OpenAI/local sections toggle within a stable window.
    m_modelLabel->setVisible(true);
    m_apiKeyLabel->setVisible(true);
    m_baseUrlLabel->setVisible(true);
    m_modelEdit->setVisible(true);
    m_baseUrlEdit->setVisible(true);
    m_apiKeyEdit->setVisible(true);
    m_openAiRefreshButton->setVisible(true);
    m_localConfigWidget->setVisible(true);
    layout->activate();
    adjustSize();
    const auto maxSize = size();

    onProviderChanged(m_providerCombo->currentIndex());
    maybeAutoRefreshOpenAiModels();
    setMinimumSize(maxSize);
    resize(maxSize);
}

void CoproducerSettingsDialog::onProviderChanged(int index)
{
    if (!m_modelLabel || !m_apiKeyLabel)
        return;

    const bool local = m_providerCombo->itemData(index).toString() == QStringLiteral("llama");
    m_modelLabel->setVisible(!local);
    m_apiKeyLabel->setVisible(!local);
    m_baseUrlLabel->setVisible(!local);
    m_modelEdit->setVisible(!local);
    m_baseUrlEdit->setVisible(!local);
    m_apiKeyEdit->setVisible(!local);
    m_openAiRefreshButton->setVisible(!local);
    m_localConfigWidget->setVisible(local);
}

void CoproducerSettingsDialog::onModelRowClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    const int row = index.row();
    if (!m_model.downloaded(row)) {
        QMessageBox dialog(QMessageBox::Question,
                           tr("Download Model"),
                           tr("Are you sure you want to download %1?\n%2 of storage will be used")
                               .arg(m_model.getName(row))
                               .arg(m_model.getFormattedDataSize(row)),
                           QMessageBox::No | QMessageBox::Yes,
                           this);
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        dialog.setWindowModality(QmlApplication::dialogModality());
        if (dialog.exec() == QMessageBox::Yes)
            downloadModel(row);
    }

    setCurrentModel(row);
}

void CoproducerSettingsDialog::onModelContextMenu(const QPoint &pos)
{
    const auto index = m_modelTable->indexAt(pos);
    if (!index.isValid() || !m_model.downloaded(index.row())) {
        updateModelStatus();
        return;
    }

    const int row = index.row();
    QMenu menu(this);
    auto deleteAction = new QAction(tr("Delete Model"), &menu);
    deleteAction->setIcon(
        QIcon::fromTheme("edit-delete", QIcon(":/icons/oxygen/32x32/actions/edit-delete.png")));
    connect(deleteAction, &QAction::triggered, this, [this, row]() { m_model.deleteFile(row); });
    menu.addAction(deleteAction);

    auto copyAction = new QAction(tr("Copy Model URL to Clipboard"), &menu);
    copyAction->setIcon(
        QIcon::fromTheme("edit-copy", QIcon(":/icons/oxygen/32x32/actions/edit-copy.png")));
    connect(copyAction, &QAction::triggered, this, [this, row]() {
        QGuiApplication::clipboard()->setText(m_model.url(row));
    });
    menu.addAction(copyAction);

    menu.exec(QCursor::pos());
    updateModelStatus();
}

// Re-highlights whichever row (if any) matches the currently configured
// local model, e.g. after the list is (re)populated or a model is deleted.
void CoproducerSettingsDialog::updateModelStatus()
{
    m_modelTable->setCurrentIndex(m_model.getIndexForPath(Settings.coproducerModelPath()));
}

void CoproducerSettingsDialog::onBrowseModelClicked()
{
    const auto path = QFileDialog::getOpenFileName(this,
                                                   tr("Find a local GGUF model"),
                                                   Settings.coproducerModelPath(),
                                                   QStringLiteral(
                                                       "GGUF models (*.gguf);;All files (*)"),
                                                   nullptr,
                                                   Util::getFileDialogOptions());
    if (!path.isEmpty()) {
        Settings.setCoproducerModelPath(path);
        setLocalModelPath(path);
    }
}

void CoproducerSettingsDialog::onRefreshModelsClicked()
{
    m_model.load(QmlExtension::LLAMA_ID);
}

void CoproducerSettingsDialog::onRefreshOpenAiModelsClicked()
{
    refreshOpenAiModels(/*silent*/ false);
}

// Guards the automatic (typing-triggered) refresh so it doesn't hit the API
// with obviously-incomplete input or repeat an identical prior request.
void CoproducerSettingsDialog::maybeAutoRefreshOpenAiModels()
{
    const auto apiKey = m_apiKeyEdit->text().trimmed();
    const auto baseUrl = currentBaseUrl();
    // Wait until an in-progress refresh or test finishes without recording
    // this attempt, so a later edit producing the same key can still refresh.
    if (m_activeReply || m_testProvider)
        return;
    if (apiKey.length() < 20 || !Util::isAllowedApiBaseUrl(baseUrl))
        return;
    const auto key = baseUrl + QChar('\n') + apiKey;
    if (key == m_lastAutoRefreshKey)
        return;
    m_lastAutoRefreshKey = key;
    refreshOpenAiModels(/*silent*/ true);
}

// Cancel stays enabled to cancel the refresh or test.
void CoproducerSettingsDialog::setBusy(bool busy)
{
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!busy);
    m_openAiRefreshButton->setEnabled(!busy);
    m_testButton->setEnabled(!busy);
    m_providerCombo->setEnabled(!busy);
    m_baseUrlEdit->setEnabled(!busy);
    m_modelEdit->setEnabled(!busy);
    m_apiKeyEdit->setEnabled(!busy);
    m_localConfigWidget->setEnabled(!busy);
}

void CoproducerSettingsDialog::reject()
{
    if (m_activeReply) {
        m_activeCanceled = true;
        m_activeReply->abort();
        return;
    }
    if (m_testProvider) {
        finishLocalTest();
        return;
    }
    QDialog::reject();
}

// A friendly first line suggesting the likely cause, then details for
// debugging connection problems.
QString CoproducerSettingsDialog::networkErrorMessage(QNetworkReply *reply,
                                                      const QByteArray &body) const
{
    QString friendly;
    switch (reply->error()) {
    case QNetworkReply::ConnectionRefusedError:
        friendly = tr("The connection was refused. Check that the server is running and that the "
                      "port number is correct.");
        break;
    case QNetworkReply::RemoteHostClosedError:
        friendly = tr("The server closed the connection unexpectedly.");
        break;
    case QNetworkReply::HostNotFoundError:
        friendly = tr("The host was not found. Check the host name in the base URL.");
        break;
    case QNetworkReply::TimeoutError:
    // setTransferTimeout() reports aborted stalled requests as canceled.
    case QNetworkReply::OperationCanceledError:
        friendly = tr("The request timed out. Check the address and port number, and check for a "
                      "firewall blocking the connection.");
        break;
    case QNetworkReply::SslHandshakeFailedError:
        friendly = tr("The secure connection failed. The server may not support https on this "
                      "port.");
        break;
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
    case QNetworkReply::ProxyAuthenticationRequiredError:
        friendly = tr("There is a problem with the network proxy.");
        break;
    case QNetworkReply::AuthenticationRequiredError:
        friendly = tr("Authentication failed. Check the API key.");
        break;
    case QNetworkReply::ContentAccessDenied:
        friendly = tr("Access was denied. Check the API key and its permissions.");
        break;
    case QNetworkReply::ContentNotFoundError:
        friendly = tr("Not found. Check the path in the base URL (it usually ends with /v1).");
        break;
    default:
        friendly = reply->errorString();
        break;
    }

    auto message = friendly;
    message += QStringLiteral("\n\nURL: %1").arg(reply->request().url().toString());
    message += QStringLiteral("\n%1 (%2)").arg(reply->errorString()).arg(int(reply->error()));
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status) {
        const auto reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        message += QStringLiteral("\nHTTP %1 %2").arg(status).arg(reason);
    }
    auto serverMessage = QJsonDocument::fromJson(body)
                             .object()
                             .value("error")
                             .toObject()
                             .value("message")
                             .toString();
    if (serverMessage.isEmpty())
        serverMessage = QString::fromUtf8(body.left(300)).trimmed();
    if (!serverMessage.isEmpty())
        message += QStringLiteral("\n%1").arg(serverMessage);
    return message;
}

// A warning box whose text can be copied for troubleshooting.
void CoproducerSettingsDialog::showErrorMessage(const QString &title, const QString &text)
{
    QMessageBox dialog(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
    dialog.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.exec();
}

void CoproducerSettingsDialog::onTestClicked()
{
    if (m_activeReply || m_testProvider)
        return;
    if (m_providerCombo->currentData().toString() == QStringLiteral("llama"))
        testLocalModel();
    else
        testOpenAi();
}

// Sends a tiny real chat completion so the test exercises the base URL, API
// key, and model end to end -- exactly what sending a prompt does.
void CoproducerSettingsDialog::testOpenAi()
{
    const auto apiKey = m_apiKeyEdit->text().trimmed();
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, tr("Test"), tr("Enter an API key first."));
        return;
    }
    const auto baseUrl = currentBaseUrl();
    if (!Util::isAllowedApiBaseUrl(baseUrl)) {
        QMessageBox::warning(this, tr("Test"), tr("Use https, or http for localhost only."));
        return;
    }
    const auto model = currentModelId();
    if (model.isEmpty()) {
        QMessageBox::warning(this, tr("Test"), tr("Enter or choose a model first."));
        return;
    }

    QNetworkRequest request{QUrl(baseUrl + QStringLiteral("/chat/completions"))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey.toUtf8());
    request.setTransferTimeout(kTestTimeoutMs);
    QJsonObject payload;
    payload["model"] = model;
    payload["messages"] = QJsonArray{
        QJsonObject{{"role", "user"}, {"content", "Reply with only the word OK."}}};
    payload["stream"] = false;

    setBusy(true);
    m_testStatusLabel->setText(tr("Testing..."));
    m_activeCanceled = false;
    auto *reply = m_network->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_activeReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_activeReply = nullptr;
        setBusy(false);
        m_testStatusLabel->clear();
        const bool canceled = m_activeCanceled;
        m_activeCanceled = false;
        if (canceled && reply->error() == QNetworkReply::OperationCanceledError)
            return;
        const auto body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            showErrorMessage(tr("Test"), networkErrorMessage(reply, body));
            return;
        }
        const auto content = QJsonDocument::fromJson(body)
                                 .object()
                                 .value("choices")
                                 .toArray()
                                 .at(0)
                                 .toObject()
                                 .value("message")
                                 .toObject()
                                 .value("content")
                                 .toString()
                                 .trimmed();
        if (content.isEmpty()) {
            auto message = tr(
                "The server responded, but not with a chat completion. Check the base URL.");
            message += QStringLiteral("\n\nURL: %1").arg(reply->request().url().toString());
            const auto snippet = QString::fromUtf8(body.left(300)).trimmed();
            if (!snippet.isEmpty())
                message += QStringLiteral("\n%1").arg(snippet);
            showErrorMessage(tr("Test"), message);
            return;
        }
        QMessageBox::information(this,
                                 tr("Test"),
                                 tr("Success! The model replied: %1").arg(content.left(200)));
    });
}

// Tests the local model the same way the Coproducer dock uses it: start
// llama-server, wait for it to load the model, and request a tiny chat
// completion.
void CoproducerSettingsDialog::testLocalModel()
{
    const auto path = Settings.coproducerModelPath();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("Test"), tr("Select a downloaded model first."));
        return;
    }
    setBusy(true);
    m_testStatusLabel->setText(tr("Starting the local model..."));
    m_testProvider = new Coproducer::LocalLlamaServerProvider(path, this);
    connect(m_testProvider,
            &Coproducer::ChatProvider::statusMessage,
            m_testStatusLabel,
            &QLabel::setText);
    connect(m_testProvider,
            &Coproducer::ChatProvider::responseReceived,
            this,
            [this](const QString &text, const Coproducer::Usage &) {
                finishLocalTest();
                const auto reply = text.trimmed();
                if (reply.isEmpty())
                    QMessageBox::information(this, tr("Test"), tr("Success! The model replied."));
                else
                    QMessageBox::information(this,
                                             tr("Test"),
                                             tr("Success! The model replied: %1")
                                                 .arg(reply.left(200)));
            });
    connect(m_testProvider,
            &Coproducer::ChatProvider::errorOccurred,
            this,
            [this](const QString &message) {
                finishLocalTest();
                showErrorMessage(tr("Test"), message);
            });
    const QList<Coproducer::ChatMessage> history = {
        {Coproducer::ChatMessage::User, QStringLiteral("Reply with only the word OK.")}};
    m_testProvider->sendMessage(history);
}

// Deleting the provider stops its llama-server.
void CoproducerSettingsDialog::finishLocalTest()
{
    if (!m_testProvider)
        return;
    disconnect(m_testProvider, nullptr, this, nullptr);
    disconnect(m_testProvider, nullptr, m_testStatusLabel, nullptr);
    m_testProvider->cancel();
    m_testProvider->deleteLater();
    m_testProvider = nullptr;
    m_testStatusLabel->clear();
    setBusy(false);
}

void CoproducerSettingsDialog::refreshOpenAiModels(bool silent)
{
    if (m_activeReply || m_testProvider)
        return;
    const auto apiKey = m_apiKeyEdit->text().trimmed();
    if (apiKey.isEmpty()) {
        if (!silent)
            QMessageBox::warning(this, tr("Refresh Models"), tr("Enter an API key first."));
        return;
    }
    const auto baseUrl = currentBaseUrl();
    if (!Util::isAllowedApiBaseUrl(baseUrl)) {
        if (!silent)
            QMessageBox::warning(this,
                                 tr("Refresh Models"),
                                 tr("Use https, or http for localhost only."));
        return;
    }

    QNetworkRequest request{QUrl(baseUrl + QStringLiteral("/models"))};
    request.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey.toUtf8());
    request.setTransferTimeout(kRefreshTimeoutMs);
    setBusy(true);
    m_activeCanceled = false;
    auto *reply = m_network->get(request);
    m_activeReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, silent]() {
        reply->deleteLater();
        m_activeReply = nullptr;
        setBusy(false);
        const bool canceled = m_activeCanceled;
        m_activeCanceled = false;
        // The user canceled; leave the model list as it was.
        if (canceled && reply->error() == QNetworkReply::OperationCanceledError)
            return;
        const auto body = reply->readAll();
        const auto root = QJsonDocument::fromJson(body).object();

        if (reply->error() != QNetworkReply::NoError) {
            // Otherwise a failed silent (auto-triggered) refresh would leave
            // a previous provider's list in place, looking like it succeeded
            // for the newly entered URL/key when it actually didn't.
            const auto currentId = currentModelId();
            m_modelEdit->clear();
            if (!currentId.isEmpty())
                m_modelEdit->addItem(currentId, currentId);
            if (!silent)
                showErrorMessage(tr("Refresh Models"), networkErrorMessage(reply, body));
            return;
        }

        // Keep chat-capable text models; the API also lists embeddings, TTS,
        // image, and moderation models that aren't usable here.
        static const QStringList kExcludedSubstrings = {QStringLiteral("embedding"),
                                                        QStringLiteral("tts"),
                                                        QStringLiteral("whisper"),
                                                        QStringLiteral("dall-e"),
                                                        QStringLiteral("moderation"),
                                                        QStringLiteral("audio"),
                                                        QStringLiteral("image"),
                                                        QStringLiteral("transcribe"),
                                                        QStringLiteral("realtime"),
                                                        QStringLiteral("davinci"),
                                                        QStringLiteral("babbage")};
        QList<QPair<QString, qint64>> models; // id, created (Unix seconds)
        const auto data = root.value("data").toArray();
        for (const auto &value : data) {
            const auto object = value.toObject();
            const auto id = object.value("id").toString();
            if (id.isEmpty())
                continue;
            bool excluded = false;
            for (const auto &substring : kExcludedSubstrings) {
                if (id.contains(substring, Qt::CaseInsensitive)) {
                    excluded = true;
                    break;
                }
            }
            if (!excluded)
                models.append({id, object.value("created").toInteger()});
        }

        if (models.isEmpty()) {
            if (!silent)
                QMessageBox::warning(this,
                                     tr("Refresh Models"),
                                     tr("No chat models were found for this API key."));
            return;
        }

        // Build one candidate list (current setting + fetched models), then
        // de-duplicate by id so the current model never appears twice, even
        // if it's also one of the fetched models.
        const auto currentId = currentModelId();
        QList<QPair<QString, QString>> entries; // id, date (empty if unknown)
        if (!currentId.isEmpty())
            entries.append({currentId, QString()});
        for (const auto &model : models) {
            QString date;
            if (model.second > 0)
                date = QDateTime::fromSecsSinceEpoch(model.second).date().toString(Qt::ISODate);
            entries.append({model.first, date});
        }
        QList<QPair<QString, QString>> deduped;
        for (const auto &entry : entries) {
            int existing = -1;
            for (int i = 0; i < deduped.size(); ++i) {
                if (deduped.at(i).first.compare(entry.first, Qt::CaseInsensitive) == 0) {
                    existing = i;
                    break;
                }
            }
            if (existing < 0)
                deduped.append(entry);
            else if (deduped.at(existing).second.isEmpty() && !entry.second.isEmpty())
                deduped[existing] = entry; // Prefer the variant with a known date.
        }
        std::sort(deduped.begin(), deduped.end(), [](const auto &a, const auto &b) {
            return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
        });

        m_modelEdit->clear();
        for (const auto &entry : deduped) {
            // Display text is the plain id (what ends up saved/sent to the
            // API); the date is a separate role the delegate right-aligns.
            const auto row = m_modelEdit->count();
            m_modelEdit->addItem(entry.first, entry.first);
            if (!entry.second.isEmpty())
                m_modelEdit->setItemData(row, entry.second, Qt::UserRole + 1);
        }
        const auto foundIndex = m_modelEdit->findData(currentId);
        if (foundIndex >= 0)
            m_modelEdit->setCurrentIndex(foundIndex);
        else
            m_modelEdit->setEditText(currentId);
        if (!silent) {
            // Draw attention to the refreshed list as the success feedback.
            m_modelEdit->setFocus(Qt::OtherFocusReason);
            m_modelEdit->showPopup();
        }
    });
}

void CoproducerSettingsDialog::downloadModel(int index)
{
    FileDownloadDialog dialog(tr("Download Model"), this);
    dialog.start(m_model.url(index),
                 m_model.localPath(index),
                 m_model.sha256(index),
                 QStringLiteral("huggingface.co"));
}

void CoproducerSettingsDialog::setCurrentModel(int index)
{
    if (m_model.downloaded(index)) {
        const auto path = m_model.localPath(index);
        if (QFileInfo::exists(path)) {
            Settings.setCoproducerModelPath(path);
            setLocalModelPath(path);
        }
    }
}

// Shows just the file name (the full path would rarely fit and isn't
// meaningful to compare at a glance); the full path is still in the tooltip.
void CoproducerSettingsDialog::setLocalModelPath(const QString &path)
{
    m_localModelLabel->setText(QFileInfo(path).fileName());
    m_localModelLabel->setCursorPosition(0);
    m_localModelLabel->setToolTip(path);
}

// Strips the "(YYYY-MM-DD)" creation-date suffix shown in the model list,
// so the raw model id is what actually gets sent to the API and saved.
QString CoproducerSettingsDialog::currentModelId() const
{
    auto text = m_modelEdit->currentText().trimmed();
    static const QRegularExpression dateSuffix(
        QStringLiteral(" \\([0-9]{4}-[0-9]{2}-[0-9]{2}\\)$"));
    text.remove(dateSuffix);
    return text;
}

// Normalizes away a trailing slash so endpoints can be built as baseUrl + "/path".
QString CoproducerSettingsDialog::currentBaseUrl() const
{
    auto text = m_baseUrlEdit->currentText().trimmed();
    if (text.endsWith('/'))
        text.chop(1);
    return text;
}

void CoproducerSettingsDialog::onAccepted()
{
    const bool local = m_providerCombo->currentData().toString() == QStringLiteral("llama");
    if (!local && !Util::isAllowedApiBaseUrl(currentBaseUrl())) {
        QMessageBox::warning(this,
                             tr("Coproducer Settings"),
                             tr("The base URL must use https, or http for localhost only."));
        return;
    }

    Settings.setCoproducerProvider(m_providerCombo->currentData().toString());
    Settings.setCoproducerBaseUrl(currentBaseUrl());
    Settings.setCoproducerModel(currentModelId());
    Settings.setCoproducerApiKey(m_apiKeyEdit->text().trimmed());
    accept();
}
