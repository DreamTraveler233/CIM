-- 群聊
CREATE TABLE IF NOT EXISTS `groups` (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  name          VARCHAR(64) NOT NULL,
  avatar        VARCHAR(255),
  profile       VARCHAR(512),
  type          TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 1私有 2公开
  max_members   INT UNSIGNED NOT NULL DEFAULT 200,
  creator_id    BIGINT UNSIGNED NOT NULL,
  owner_id      BIGINT UNSIGNED NOT NULL,
  is_mute       TINYINT UNSIGNED NOT NULL DEFAULT 0,
  mute_until    DATETIME,
  is_overt      TINYINT UNSIGNED NOT NULL DEFAULT 0,
  visit_card    VARCHAR(64),
  notice_id     BIGINT UNSIGNED,
  status        TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 1正常 2解散
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  KEY idx_groups_owner (owner_id),
  KEY idx_groups_status (status, created_at),
  CONSTRAINT fk_groups_creator FOREIGN KEY (creator_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_groups_owner FOREIGN KEY (owner_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS group_members (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  group_id      BIGINT UNSIGNED NOT NULL,
  user_id       BIGINT UNSIGNED NOT NULL,
  role          TINYINT UNSIGNED NOT NULL DEFAULT 3, -- 1群主 2管理员 3成员
  is_mute       TINYINT UNSIGNED NOT NULL DEFAULT 0,
  mute_until    DATETIME,
  remark        VARCHAR(64),
  alias         VARCHAR(64),
  joined_at     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  last_seen_seq BIGINT UNSIGNED NOT NULL DEFAULT 0,
  status        TINYINT UNSIGNED NOT NULL DEFAULT 1,
  UNIQUE KEY uq_group_member (group_id, user_id),
  KEY idx_group_member_role (group_id, role),
  CONSTRAINT fk_group_members_group FOREIGN KEY (group_id) REFERENCES `groups` (id) ON DELETE CASCADE,
  CONSTRAINT fk_group_members_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS group_notices (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  group_id      BIGINT UNSIGNED NOT NULL,
  content       TEXT NOT NULL,
  author_id     BIGINT UNSIGNED,
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_group_notices_group FOREIGN KEY (group_id) REFERENCES `groups` (id) ON DELETE CASCADE,
  CONSTRAINT fk_group_notices_author FOREIGN KEY (author_id) REFERENCES users (id) ON DELETE SET NULL
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS group_join_requests (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  group_id      BIGINT UNSIGNED NOT NULL,
  applicant_id  BIGINT UNSIGNED NOT NULL,
  remark        VARCHAR(255),
  status        TINYINT UNSIGNED NOT NULL DEFAULT 0, -- 0待审核 1同意 2拒绝
  handler_id    BIGINT UNSIGNED,
  handle_remark VARCHAR(255),
  handled_at    DATETIME,
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  KEY idx_group_apply_status (group_id, status, created_at),
  KEY idx_group_apply_applicant (applicant_id, created_at),
  CONSTRAINT fk_group_join_group FOREIGN KEY (group_id) REFERENCES `groups` (id) ON DELETE CASCADE,
  CONSTRAINT fk_group_join_applicant FOREIGN KEY (applicant_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_group_join_handler FOREIGN KEY (handler_id) REFERENCES users (id) ON DELETE SET NULL
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS group_invitations (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  group_id      BIGINT UNSIGNED NOT NULL,
  inviter_id    BIGINT UNSIGNED NOT NULL,
  invitee_id    BIGINT UNSIGNED NOT NULL,
  status        TINYINT UNSIGNED NOT NULL DEFAULT 0, -- 0待接受 1已加入 2拒绝
  expire_at     DATETIME,
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  KEY idx_group_invite_status (group_id, status),
  CONSTRAINT fk_group_invite_group FOREIGN KEY (group_id) REFERENCES `groups` (id) ON DELETE CASCADE,
  CONSTRAINT fk_group_invite_inviter FOREIGN KEY (inviter_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_group_invite_invitee FOREIGN KEY (invitee_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS group_votes (
  id             BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  group_id       BIGINT UNSIGNED NOT NULL,
  title          VARCHAR(128) NOT NULL,
  answer_mode    TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 1单选 2多选
  is_anonymous   TINYINT UNSIGNED NOT NULL DEFAULT 0,
  created_by     BIGINT UNSIGNED NOT NULL,
  status         TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 1进行中 2结束
  created_at     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  closed_at      DATETIME,
  CONSTRAINT fk_group_votes_group FOREIGN KEY (group_id) REFERENCES `groups` (id) ON DELETE CASCADE,
  CONSTRAINT fk_group_votes_creator FOREIGN KEY (created_by) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS group_vote_options (
  id         BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  vote_id    BIGINT UNSIGNED NOT NULL,
  option_key VARCHAR(32) NOT NULL,
  content    VARCHAR(255) NOT NULL,
  sort       INT UNSIGNED NOT NULL DEFAULT 100,
  UNIQUE KEY uq_group_vote_option (vote_id, option_key),
  CONSTRAINT fk_group_vote_options_vote FOREIGN KEY (vote_id) REFERENCES group_votes (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS group_vote_records (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  vote_id     BIGINT UNSIGNED NOT NULL,
  user_id     BIGINT UNSIGNED NOT NULL,
  option_key  VARCHAR(32) NOT NULL,
  answer_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uq_group_vote_record (vote_id, user_id, option_key),
  CONSTRAINT fk_group_vote_record_vote FOREIGN KEY (vote_id) REFERENCES group_votes (id) ON DELETE CASCADE,
  CONSTRAINT fk_group_vote_record_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;
