-- 笔记（文章）模块
CREATE TABLE IF NOT EXISTS article_classifies (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id     BIGINT UNSIGNED NOT NULL,
  name        VARCHAR(64) NOT NULL,
  sort        INT UNSIGNED NOT NULL DEFAULT 100,
  is_default  TINYINT UNSIGNED NOT NULL DEFAULT 0,
  created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uq_article_classify (user_id, name),
  CONSTRAINT fk_article_classify_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS article_tags (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id     BIGINT UNSIGNED NOT NULL,
  tag_name    VARCHAR(64) NOT NULL,
  sort        INT UNSIGNED NOT NULL DEFAULT 100,
  created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uq_article_tag (user_id, tag_name),
  CONSTRAINT fk_article_tags_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS articles (
  id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id       BIGINT UNSIGNED NOT NULL,
  classify_id   BIGINT UNSIGNED,
  title         VARCHAR(128) NOT NULL,
  abstract      VARCHAR(255),
  md_content    LONGTEXT NOT NULL,
  cover_image   VARCHAR(255),
  status        TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 1正常 2草稿 3归档
  is_asterisk   TINYINT UNSIGNED NOT NULL DEFAULT 0,
  created_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  deleted_at    DATETIME,
  KEY idx_articles_classify (classify_id, status),
  KEY idx_articles_user (user_id, status),
  FULLTEXT KEY ft_articles_title_content (title, md_content),
  CONSTRAINT fk_articles_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_articles_classify FOREIGN KEY (classify_id) REFERENCES article_classifies (id) ON DELETE SET NULL
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  ROW_FORMAT=DYNAMIC;

CREATE TABLE IF NOT EXISTS article_tag_relations (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  article_id  BIGINT UNSIGNED NOT NULL,
  tag_id      BIGINT UNSIGNED NOT NULL,
  UNIQUE KEY uq_article_tag_relation (article_id, tag_id),
  CONSTRAINT fk_article_tag_rel_article FOREIGN KEY (article_id) REFERENCES articles (id) ON DELETE CASCADE,
  CONSTRAINT fk_article_tag_rel_tag FOREIGN KEY (tag_id) REFERENCES article_tags (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS article_annex_files (
  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  article_id   BIGINT UNSIGNED NOT NULL,
  file_id      BIGINT UNSIGNED NOT NULL,
  created_at   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  deleted_at   DATETIME,
  CONSTRAINT fk_article_annex_article FOREIGN KEY (article_id) REFERENCES articles (id) ON DELETE CASCADE,
  CONSTRAINT fk_article_annex_file FOREIGN KEY (file_id) REFERENCES storage_files (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS article_favorites (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id     BIGINT UNSIGNED NOT NULL,
  article_id  BIGINT UNSIGNED NOT NULL,
  created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uq_article_favorite (user_id, article_id),
  CONSTRAINT fk_article_fav_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE,
  CONSTRAINT fk_article_fav_article FOREIGN KEY (article_id) REFERENCES articles (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS article_recycle_logs (
  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  article_id   BIGINT UNSIGNED NOT NULL,
  user_id      BIGINT UNSIGNED NOT NULL,
  deleted_at   DATETIME NOT NULL,
  restored_at  DATETIME,
  action       TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 1删除 2恢复 3彻底删除
  CONSTRAINT fk_article_recycle_article FOREIGN KEY (article_id) REFERENCES articles (id) ON DELETE CASCADE,
  CONSTRAINT fk_article_recycle_user FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
) ENGINE=InnoDB;
