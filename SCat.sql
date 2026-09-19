-- SCat 数据库初始化脚本
-- 用法：mysql -u root -p < init.sql

CREATE DATABASE IF NOT EXISTS scat
    DEFAULT CHARSET utf8mb4
    COLLATE utf8mb4_general_ci;

USE scat;

-- ============ 用户表 ============
-- username 用 utf8mb4_bin，区分大小写：Kazie 和 kazie 是两个账号
CREATE TABLE IF NOT EXISTS users (
    id          INT AUTO_INCREMENT PRIMARY KEY,
    username    VARCHAR(15) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL UNIQUE,
    password    VARCHAR(64)  NOT NULL,
    nickname    VARCHAR(16)  NOT NULL,
    avatar      VARCHAR(255) NOT NULL DEFAULT 'head.png',
    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============ 好友关系表 ============
-- 一对好友存两行（A→B 和 B→A），查好友列表只要一个条件
CREATE TABLE IF NOT EXISTS friends (
    id          INT AUTO_INCREMENT PRIMARY KEY,
    username    VARCHAR(15) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
    friend      VARCHAR(15) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
    remark      VARCHAR(16) DEFAULT NULL,
    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,

    UNIQUE KEY uk_pair (username, friend),
    FOREIGN KEY (username) REFERENCES users(username) ON DELETE CASCADE,
    FOREIGN KEY (friend)   REFERENCES users(username) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============ 好友申请表 ============
-- uk_pair 保证一对人之间只有一条记录，重新申请是把它重置成待处理
CREATE TABLE IF NOT EXISTS friend_request (
    id         BIGINT AUTO_INCREMENT PRIMARY KEY,
    sender     VARCHAR(15) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
    receiver   VARCHAR(15) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
    message    VARCHAR(64) DEFAULT NULL,
    status     TINYINT NOT NULL DEFAULT 0,      -- 0=待处理 1=已同意 2=已拒绝
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    handled_at DATETIME DEFAULT NULL,

    UNIQUE KEY uk_pair (sender, receiver),
    KEY idx_receiver_status (receiver, status),
    FOREIGN KEY (sender)   REFERENCES users(username) ON DELETE CASCADE,
    FOREIGN KEY (receiver) REFERENCES users(username) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============ 离线消息暂存表 ============
-- 只是中转站，客户端确认收到后就删，不会越长越大
CREATE TABLE IF NOT EXISTS offline_msg (
    id         BIGINT AUTO_INCREMENT PRIMARY KEY,
    msgid      CHAR(36) NOT NULL UNIQUE,
    sender     VARCHAR(15) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
    receiver   VARCHAR(15) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
    content    MEDIUMTEXT   NOT NULL,
    send_time  BIGINT NOT NULL,
    kind       TINYINT NOT NULL DEFAULT 0,
    image      MEDIUMTEXT DEFAULT NULL,
    img_w      INT NOT NULL DEFAULT 0,
    img_h      INT NOT NULL DEFAULT 0,
    file_id    CHAR(36) DEFAULT NULL,
    file_name  VARCHAR(255) DEFAULT NULL,
    file_size  BIGINT NOT NULL DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,

    KEY idx_receiver (receiver),
    FOREIGN KEY (sender)   REFERENCES users(username) ON DELETE CASCADE,
    FOREIGN KEY (receiver) REFERENCES users(username) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS file_store (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY,
    file_id     CHAR(36) NOT NULL UNIQUE,
    sender      VARCHAR(15) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
    receiver    VARCHAR(15) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
    file_name   VARCHAR(255) NOT NULL,
    file_size   BIGINT NOT NULL,
    -- 0 = 还在传（或者传到一半断了）1 = 完整收好了。
    -- 只有 finished=1 的才允许下载，否则会下到半截文件
    finished    TINYINT NOT NULL DEFAULT 0,
    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,

    KEY idx_created (created_at),
    FOREIGN KEY (sender)   REFERENCES users(username) ON DELETE CASCADE,
    FOREIGN KEY (receiver) REFERENCES users(username) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;