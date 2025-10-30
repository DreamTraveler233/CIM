-- 自定义表情
CREATE TABLE IF NOT EXISTS user_emoticons (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id     BIGINT UNSIGNED NOT NULL,
  file_id     BIGINT UNSIGNED,
  remote_url  VARCHAR(255),
  drive       TINYINT UNSIGNED NOT NULL DEFAULT 1,
  name        VARCHAR(64),
  created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uq_user_emoticon (user_id, remote_url, file_id),
  CONSTRAINT fk_user_emoticon_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_user_emoticon_file FOREIGN KEY (file_id) REFERENCES storage_files (id) ON DELETE SET NULL
) ENGINE=InnoDB;
