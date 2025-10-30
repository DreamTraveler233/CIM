-- 会话与消息
CREATE TABLE IF NOT EXISTS conversations (
  id              BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  talk_mode       TINYINT UNSIGNED NOT NULL,              -- 1私聊 2群聊
  conversation_key VARCHAR(64) NOT NULL,                  -- 例如 1:uid1:uid2 或 2:group_id
  single_user_low BIGINT UNSIGNED,
  single_user_high BIGINT UNSIGNED,
  group_id        BIGINT UNSIGNED,
  created_at      DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uq_conversation_key (conversation_key),
  KEY idx_conversation_mode (talk_mode),
  CONSTRAINT fk_conversation_group FOREIGN KEY (group_id) REFERENCES `groups` (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS talk_sessions (
  id               BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id          BIGINT UNSIGNED NOT NULL,
  conversation_id  BIGINT UNSIGNED NOT NULL,
  talk_mode        TINYINT UNSIGNED NOT NULL,
  target_user_id   BIGINT UNSIGNED,
  target_group_id  BIGINT UNSIGNED,
  is_top           TINYINT UNSIGNED NOT NULL DEFAULT 0,
  is_disturb       TINYINT UNSIGNED NOT NULL DEFAULT 0,
  is_robot         TINYINT UNSIGNED NOT NULL DEFAULT 0,
  unread_count     INT UNSIGNED NOT NULL DEFAULT 0,
  last_msg_id      VARCHAR(64),
  last_msg_preview VARCHAR(255),
  last_active_at   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  created_at       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uq_session_user_conversation (user_id, conversation_id),
  KEY idx_session_unread (user_id, unread_count),
  CONSTRAINT fk_talk_sessions_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_talk_sessions_conversation FOREIGN KEY (conversation_id) REFERENCES conversations (id) ON DELETE CASCADE,
  CONSTRAINT fk_talk_sessions_target_user FOREIGN KEY (target_user_id) REFERENCES users (id) ON DELETE SET NULL,
  CONSTRAINT fk_talk_sessions_target_group FOREIGN KEY (target_group_id) REFERENCES `groups` (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS messages (
  id               BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  conversation_id  BIGINT UNSIGNED NOT NULL,
  msg_id           VARCHAR(64) NOT NULL,
  sequence         BIGINT UNSIGNED NOT NULL,
  msg_type         INT UNSIGNED NOT NULL,
  sender_id        BIGINT UNSIGNED NOT NULL,
  receiver_id      BIGINT UNSIGNED,
  group_id         BIGINT UNSIGNED,
  status           TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 1成功 2发送中 3失败
  is_revoked       TINYINT UNSIGNED NOT NULL DEFAULT 0,
  quote_msg_id     VARCHAR(64),
  body_json        JSON NOT NULL,
  send_time        DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uq_messages_msg_id (msg_id),
  UNIQUE KEY uq_messages_conversation_seq (conversation_id, sequence),
  KEY idx_messages_sender (sender_id, send_time),
  KEY idx_messages_receiver (receiver_id, send_time),
  KEY idx_messages_group (group_id, send_time),
  CONSTRAINT fk_messages_conversation FOREIGN KEY (conversation_id) REFERENCES conversations (id) ON DELETE CASCADE,
  CONSTRAINT fk_messages_sender FOREIGN KEY (sender_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_messages_receiver FOREIGN KEY (receiver_id) REFERENCES users (id) ON DELETE SET NULL,
  CONSTRAINT fk_messages_group FOREIGN KEY (group_id) REFERENCES `groups` (id) ON DELETE CASCADE,
  CONSTRAINT fk_messages_quote FOREIGN KEY (quote_msg_id) REFERENCES messages (msg_id) ON DELETE SET NULL
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS message_recipients (
  id             BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  msg_id         VARCHAR(64) NOT NULL,
  user_id        BIGINT UNSIGNED NOT NULL,
  conversation_id BIGINT UNSIGNED NOT NULL,
  receive_status TINYINT UNSIGNED NOT NULL DEFAULT 0, -- 0未读 1已读
  is_deleted     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  read_at        DATETIME,
  deleted_at     DATETIME,
  UNIQUE KEY uq_message_recipient (msg_id, user_id),
  KEY idx_message_recipient_user (user_id, receive_status),
  CONSTRAINT fk_message_recipient_msg FOREIGN KEY (msg_id) REFERENCES messages (msg_id) ON DELETE CASCADE,
  CONSTRAINT fk_message_recipient_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_message_recipient_conversation FOREIGN KEY (conversation_id) REFERENCES conversations (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS message_attachments (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  msg_id        VARCHAR(64) NOT NULL,
  file_id       BIGINT UNSIGNED NOT NULL,
  attachment_type TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 1文件 2图片 3音频 4视频
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uq_message_attachment (msg_id, file_id),
  CONSTRAINT fk_message_attachment_msg FOREIGN KEY (msg_id) REFERENCES messages (msg_id) ON DELETE CASCADE,
  CONSTRAINT fk_message_attachment_file FOREIGN KEY (file_id) REFERENCES storage_files (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS forwarded_messages (
  id             BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  root_msg_id    VARCHAR(64) NOT NULL,
  child_msg_id   VARCHAR(64) NOT NULL,
  created_at     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uq_forward_relation (root_msg_id, child_msg_id),
  CONSTRAINT fk_forward_root FOREIGN KEY (root_msg_id) REFERENCES messages (msg_id) ON DELETE CASCADE,
  CONSTRAINT fk_forward_child FOREIGN KEY (child_msg_id) REFERENCES messages (msg_id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS mixed_message_items (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  msg_id        VARCHAR(64) NOT NULL,
  item_order    INT UNSIGNED NOT NULL DEFAULT 1,
  item_type     INT UNSIGNED NOT NULL,
  content       TEXT,
  link_url      VARCHAR(255),
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uq_mixed_item (msg_id, item_order),
  CONSTRAINT fk_mixed_item_msg FOREIGN KEY (msg_id) REFERENCES messages (msg_id) ON DELETE CASCADE
) ENGINE=InnoDB;
