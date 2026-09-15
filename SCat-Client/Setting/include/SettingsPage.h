#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

// 设置页。嵌在 ScatWindow 右侧的 QStackedWidget 里，
// 跟聊天页是平级的一页，不是弹窗
class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);
    ~SettingsPage();

    // --- 给逻辑层调用的接口 ---
    void setUserInfo(const QString& username, const QString& nickname,const QString& avatar);
    void setAvatar(const QString& avatar);        // 上传成功后刷新显示
    void setNickname(const QString& nickname);
    void setStoragePath(const QString& path);

    void onNicknameSaved(bool ok, const QString& reason);
    void onAvatarUploaded(bool ok, const QString& reason);

signals:
    void sendChangeAvatar(const QString& filePath);  // 用户选好了图片文件
    void sendSaveNickname(const QString& nickname);
    void sendChangeStorage(const QString& dir);      // 用户选好了新目录

private slots:
    void onAvatarBtnClicked();
    void onSaveNicknameClicked();
    void onChangeStorageClicked();

private:
    void initUI();
    void initConnect();

    QWidget* createCard();                           // 统一的白色圆角卡片
    QWidget* createRow(const QString& title, QWidget* right);   // 一行：左标题右内容

private:
    // 顶栏
    QWidget* headerWidget;
    QLabel* lbHeaderTitle;

    // 头像卡片
    QLabel* lbAvatar;
    QPushButton* btnChangeAvatar;

    // 账号 / 昵称卡片
    QLabel* lbUsername;
    QLineEdit* editNickname;
    QPushButton* btnSaveNickname;

    // 存储位置卡片
    QLabel* lbStoragePath;
    QPushButton* btnChangeStorage;

    QString fullStoragePath;    // 完整路径，界面上显示的是省略版
};

#endif // SETTINGSPAGE_H