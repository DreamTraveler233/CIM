-- 联系人与申请
CREATE TABLE IF NOT EXISTS contact_groups (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id       BIGINT UNSIGNED NOT NULL,
  name          VARCHAR(32) NOT NULL,
  sort          INT UNSIGNED NOT NULL DEFAULT 100,
  contact_count INT UNSIGNED NOT NULL DEFAULT 0,
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uq_contact_group_user_name (user_id, name),
  KEY idx_contact_group_sort (user_id, sort),
  CONSTRAINT fk_contact_groups_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS contacts (
  id             BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id        BIGINT UNSIGNED NOT NULL,
  contact_id     BIGINT UNSIGNED NOT NULL,
  group_id       BIGINT UNSIGNED,
  remark         VARCHAR(64),
  relation       TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 1好友 2黑名单 3拉黑
  status         TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 1正常 2删除
  created_at     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uq_contact_pair (user_id, contact_id),
  KEY idx_contacts_group (user_id, group_id),
  CONSTRAINT fk_contacts_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_contacts_contact FOREIGN KEY (contact_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_contacts_group FOREIGN KEY (group_id) REFERENCES contact_groups (id) ON DELETE SET NULL
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS contact_applies (
  id             BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  applicant_id   BIGINT UNSIGNED NOT NULL,
  target_id      BIGINT UNSIGNED NOT NULL,
  remark         VARCHAR(255),
  status         TINYINT UNSIGNED NOT NULL DEFAULT 0, -- 0待处理 1同意 2拒绝
  handler_id     BIGINT UNSIGNED,
  handle_remark  VARCHAR(255),
  handled_at     DATETIME,
  created_at     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  KEY idx_contact_apply_status (target_id, status, created_at),
  KEY idx_contact_apply_applicant (applicant_id, created_at),
  CONSTRAINT fk_contact_apply_applicant FOREIGN KEY (applicant_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_contact_apply_target FOREIGN KEY (target_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_contact_apply_handler FOREIGN KEY (handler_id) REFERENCES users (id) ON DELETE SET NULL
) ENGINE=InnoDB;
