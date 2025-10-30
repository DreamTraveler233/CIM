-- 企业组织
CREATE TABLE IF NOT EXISTS organize_departments (
  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  parent_id    BIGINT UNSIGNED,
  dept_name    VARCHAR(128) NOT NULL,
  ancestors    VARCHAR(255),
  sort         INT UNSIGNED NOT NULL DEFAULT 100,
  created_at   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_department_parent FOREIGN KEY (parent_id) REFERENCES organize_departments (id) ON DELETE SET NULL
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS organize_personnel (
  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id      BIGINT UNSIGNED NOT NULL,
  dept_id      BIGINT UNSIGNED NOT NULL,
  position     VARCHAR(128),
  is_leader    TINYINT UNSIGNED NOT NULL DEFAULT 0,
  joined_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  status       TINYINT UNSIGNED NOT NULL DEFAULT 1,
  CONSTRAINT fk_personnel_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_personnel_dept FOREIGN KEY (dept_id) REFERENCES organize_departments (id) ON DELETE CASCADE,
  UNIQUE KEY uq_personnel_user_dept (user_id, dept_id)
) ENGINE=InnoDB;
