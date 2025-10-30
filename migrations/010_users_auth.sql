-- 用户与认证
CREATE TABLE IF NOT EXISTS users (
  id                BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  mobile            VARCHAR(20) NOT NULL,
  email             VARCHAR(128),
  nickname          VARCHAR(64) NOT NULL,
  password_hash     VARCHAR(255) NOT NULL,
  password_salt     VARCHAR(64) NOT NULL,
  avatar            VARCHAR(255),
  motto             VARCHAR(255),
  gender            TINYINT UNSIGNED NOT NULL DEFAULT 0,   -- 0未知 1男 2女
  is_robot          TINYINT UNSIGNED NOT NULL DEFAULT 0,
  is_qiye           TINYINT UNSIGNED NOT NULL DEFAULT 0,
  status            TINYINT UNSIGNED NOT NULL DEFAULT 1,   -- 1正常 2禁用
  last_login_at     DATETIME,
  created_at        DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at        DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uq_users_mobile (mobile),
  UNIQUE KEY uq_users_email (email)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS user_settings (
  user_id              BIGINT UNSIGNED PRIMARY KEY,
  theme_mode           VARCHAR(16) NOT NULL DEFAULT 'light',
  theme_bag_img        VARCHAR(255),
  theme_color          VARCHAR(16) NOT NULL DEFAULT '#409eff',
  notify_cue_tone      VARCHAR(64) NOT NULL DEFAULT 'default',
  keyboard_event_notify VARCHAR(8) NOT NULL DEFAULT 'N',
  created_at           DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at           DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_user_settings_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS user_tokens (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id       BIGINT UNSIGNED NOT NULL,
  access_token  CHAR(64) NOT NULL,
  platform      VARCHAR(32) NOT NULL DEFAULT 'web',
  client_ip     VARCHAR(45),
  expires_at    DATETIME NOT NULL,
  revoked_at    DATETIME,
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uq_user_tokens_token (access_token),
  KEY idx_user_tokens_user (user_id, platform, expires_at),
  CONSTRAINT fk_user_tokens_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS user_oauth_accounts (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id       BIGINT UNSIGNED NOT NULL,
  provider      VARCHAR(32) NOT NULL,
  open_id       VARCHAR(128) NOT NULL,
  union_id      VARCHAR(128),
  access_token  VARCHAR(255),
  refresh_token VARCHAR(255),
  expires_at    DATETIME,
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uq_user_oauth (provider, open_id),
  KEY idx_user_oauth_user (user_id, provider),
  CONSTRAINT fk_user_oauth_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS user_login_logs (
  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id      BIGINT UNSIGNED NOT NULL,
  platform     VARCHAR(32) NOT NULL,
  login_ip     VARCHAR(45),
  login_agent  VARCHAR(255),
  login_city   VARCHAR(128),
  login_at     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  status       TINYINT UNSIGNED NOT NULL DEFAULT 1,
  remark       VARCHAR(255),
  KEY idx_user_login_logs_user (user_id, login_at),
  CONSTRAINT fk_user_login_logs_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS sms_verification_codes (
  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  mobile       VARCHAR(20) NOT NULL,
  code         VARCHAR(10) NOT NULL,
  channel      VARCHAR(32) NOT NULL,
  status       TINYINT UNSIGNED NOT NULL DEFAULT 0, -- 0待验证 1成功 2失效
  expired_at   DATETIME NOT NULL,
  used_at      DATETIME,
  send_ip      VARCHAR(45),
  created_at   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  KEY idx_sms_mobile_channel (mobile, channel, status),
  KEY idx_sms_expired_at (expired_at)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS email_verification_codes (
  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  email        VARCHAR(128) NOT NULL,
  code         VARCHAR(10) NOT NULL,
  channel      VARCHAR(32) NOT NULL,
  status       TINYINT UNSIGNED NOT NULL DEFAULT 0,
  expired_at   DATETIME NOT NULL,
  used_at      DATETIME,
  send_ip      VARCHAR(45),
  created_at   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  KEY idx_email_channel (email, channel, status),
  KEY idx_email_expired_at (expired_at)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS storage_files (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id       BIGINT UNSIGNED,
  drive         TINYINT UNSIGNED NOT NULL DEFAULT 1,  -- 1本地 2OSS等
  file_name     VARCHAR(255) NOT NULL,
  file_ext      VARCHAR(32),
  mime_type     VARCHAR(128),
  file_size     BIGINT UNSIGNED NOT NULL DEFAULT 0,
  file_path     VARCHAR(512) NOT NULL,
  md5_hash      CHAR(32),
  sha1_hash     CHAR(40),
  status        TINYINT UNSIGNED NOT NULL DEFAULT 1,
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  deleted_at    DATETIME,
  KEY idx_storage_user (user_id, status),
  KEY idx_storage_hash (md5_hash),
  CONSTRAINT fk_storage_files_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE SET NULL
) ENGINE=InnoDB;
