#ifndef BAIDU_OCR_H
#define BAIDU_OCR_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>

/**
 * @brief 百度车牌识别类
 * 封装百度OCR接口，实现车牌自动识别
 */
class BaiduLicensePlateOCR : public QObject
{
    Q_OBJECT
public:
    explicit BaiduLicensePlateOCR(QObject *parent = nullptr);

    // 设置API Key和Secret Key
    void setApiKey(const QString &key);
    void setSecretKey(const QString &key);

    /**
     * @brief 识别图片中的车牌信息
     * @param imageData 图片数据
     */
    void recognizeLicensePlate(const QByteArray &imageData);

signals:
    // 识别完成，返回车牌号
    void recognitionFinished(const QString &plate);
    // 识别失败，返回错误信息
    void recognitionError(const QString &error);

private slots:
    void onAccessTokenReply(QNetworkReply *reply);  // Token获取回调
    void onOcrReply(QNetworkReply *reply);          // OCR识别回调

private:
    void requestAccessToken();                      // 请求Token

private:
    QNetworkAccessManager *manager;
    QString apiKey;
    QString secretKey;
    QString accessToken;

    QByteArray pendingImage;  // 用于缓存第一次识别传入的图片
};

#endif // BAIDU_OCR_H
