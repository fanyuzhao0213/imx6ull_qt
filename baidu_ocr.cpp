#include "baidu_ocr.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QUrl>

BaiduLicensePlateOCR::BaiduLicensePlateOCR(QObject *parent)
    : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
}

void BaiduLicensePlateOCR::setApiKey(const QString &key)
{
    apiKey = key;
}

void BaiduLicensePlateOCR::setSecretKey(const QString &key)
{
    secretKey = key;
}

void BaiduLicensePlateOCR::recognizeLicensePlate(const QByteArray &imageData)
{
    if (imageData.isEmpty()) {
        emit recognitionError("图片数据为空");
        return;
    }

    // 如果没有token，先缓存图片，再请求token
    if (accessToken.isEmpty()) {
        pendingImage = imageData;
        requestAccessToken();
        return;
    }

    // 直接识别车牌
    QUrl ocrUrl(QString("https://aip.baidubce.com/rest/2.0/ocr/v1/license_plate?access_token=%1")
                .arg(accessToken));
    QNetworkRequest request(ocrUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray postData;
    postData.append("image=" + QUrl::toPercentEncoding(imageData.toBase64()));

    QNetworkReply *reply = manager->post(request, postData);
    connect(reply, &QNetworkReply::finished, this, [=]() { onOcrReply(reply); });
}

void BaiduLicensePlateOCR::requestAccessToken()
{
     //设置百度 OAuth2.0 token 获取地址
    QUrl tokenUrl("https://aip.baidubce.com/oauth/2.0/token");
    QUrlQuery query;
    //固定值 client_credentials，表示使用 API Key + Secret Key 直接换取 access_token。
    //百度 API 目前仅支持这种方式
    query.addQueryItem("grant_type", "client_credentials");
    query.addQueryItem("client_id", apiKey);
    query.addQueryItem("client_secret", secretKey);
    tokenUrl.setQuery(query);

    /*
    https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials
    &client_id=MG3WO7c6x7AztGyakAucyLqm
    &client_secret=AnEAgOFdcKJ6qCvXPLlXNjlDiDXpa5uT
    */
    QNetworkRequest request(tokenUrl);
    // 发起 GET 请求，获取 token
    QNetworkReply *reply = manager->get(request);
    // 连接响应信号，等待请求完成
    connect(reply, &QNetworkReply::finished, this, [=]() { onAccessTokenReply(reply); });
}

void BaiduLicensePlateOCR::onAccessTokenReply(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit recognitionError("Access token JSON解析失败");
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("access_token")) {
        accessToken = obj.value("access_token").toString();
        qDebug() << "[BaiduOCR] Access token获取成功:" << accessToken;

        // 如果之前有待处理的图片，自动重新调用识别
        if (!pendingImage.isEmpty()) {
            QByteArray img = pendingImage;
            pendingImage.clear();
            recognizeLicensePlate(img);
        }
    } else {
        emit recognitionError("获取Access token失败: " + data);
    }
}

void BaiduLicensePlateOCR::onOcrReply(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit recognitionError("OCR JSON解析失败");
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("words_result")) {
        QJsonObject wordsResult = obj.value("words_result").toObject();
        QString plate = wordsResult.value("number").toString();
        emit recognitionFinished(plate.isEmpty() ? "识别失败" : plate);
    } else {
        emit recognitionError("OCR接口返回无效: " + QString::fromUtf8(data));
    }
}
