#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MIGRATIONS_DIR="$ROOT_DIR/migrations"
CONFIG_FILE="$ROOT_DIR/bin/config/system.yaml"

CMD="${1:-}"; shift || true

DB_HOST=""
DB_PORT=""
DB_USER=""
DB_PASS=""
DB_NAME=""

usage() {
  cat <<EOF
Usage: scripts/sql/migrate.sh <command> [options]

Commands:
  list                   列出所有迁移文件及其应用状态
  status                 显示迁移状态摘要
  apply                  应用所有未执行的迁移文件

Options:
  --host <host>          数据库主机 (默认读取 bin/config/system.yaml)
  --port <port>          数据库端口 (默认读取 bin/config/system.yaml)
  --user <user>          数据库用户 (默认读取 bin/config/system.yaml)
  --pass <pass>          数据库密码 (默认读取 bin/config/system.yaml)
  --name <name>          数据库名   (默认读取 bin/config/system.yaml)

示例:
  scripts/sql/migrate.sh list
  scripts/sql/migrate.sh apply --host 127.0.0.1 --port 3306 --user root --pass 123 --name cim_db
EOF
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --host) DB_HOST="$2"; shift 2;;
      --port) DB_PORT="$2"; shift 2;;
      --user) DB_USER="$2"; shift 2;;
      --pass) DB_PASS="$2"; shift 2;;
      --name) DB_NAME="$2"; shift 2;;
      *) echo "未知参数: $1"; usage; exit 1;;
    esac
  done
}

trim() { sed -e 's/^\s*//' -e 's/\s*$//'; }

read_config_defaults() {
  # 从 bin/config/system.yaml 读取 mysql.dbs -> default 配置
  if [[ -z "$DB_HOST" || -z "$DB_PORT" || -z "$DB_USER" || -z "$DB_PASS" || -z "$DB_NAME" ]]; then
    if [[ ! -f "$CONFIG_FILE" ]]; then
      echo "未找到配置文件: $CONFIG_FILE" >&2
      exit 1
    fi
    # awk 解析缩进块：mysql.dbs: -> default: -> k:v
    local vals
    vals=$(awk '
      $1=="mysql.dbs:"{s=1; next}
      s && $1=="default:"{d=1; next}
      s && d {
        if (substr($0,1,8)=="        ") {
          line=$0
          # 去掉前导空白
          sub(/^\s+/, "", line)
          split(line, a, ":")
          key=a[1]
          val=substr(line, index(line, ":")+1)
          gsub(/^\s+|\s+$/, "", key)
          gsub(/^\s+|\s+$/, "", val)
          gsub(/^"|"$/, "", val)
          kv[key]=val
        } else if (substr($0,1,4)!="    ") {
          # 退出 default 块
          s=0; d=0
        }
      }
      END{
        # 输出顺序: host port user passwd dbname
        print kv["host"], kv["port"], kv["user"], kv["passwd"], kv["dbname"]
      }
    ' "$CONFIG_FILE" | tr -s ' ')

    # shellcheck disable=SC2206
    local arr=($vals)
    [[ -z "$DB_HOST" && -n "${arr[0]:-}" ]] && DB_HOST="${arr[0]}"
    [[ -z "$DB_PORT" && -n "${arr[1]:-}" ]] && DB_PORT="${arr[1]}"
    [[ -z "$DB_USER" && -n "${arr[2]:-}" ]] && DB_USER="${arr[2]}"
    [[ -z "$DB_PASS" && -n "${arr[3]:-}" ]] && DB_PASS="${arr[3]}"
    [[ -z "$DB_NAME" && -n "${arr[4]:-}" ]] && DB_NAME="${arr[4]}"

    # fallback: 若 awk 解析失败，按 8 空格缩进的键名直接抓取
    if [[ -z "$DB_HOST" || -z "$DB_PORT" || -z "$DB_USER" || -z "$DB_NAME" ]]; then
      local line
      line=$(grep -E "^\\s{8}host:" -m1 "$CONFIG_FILE" || true); [[ -n "$line" ]] && DB_HOST=$(echo "$line" | awk -F: '{print $2}' | sed 's/[\" ]//g')
      line=$(grep -E "^\\s{8}port:" -m1 "$CONFIG_FILE" || true); [[ -n "$line" ]] && DB_PORT=$(echo "$line" | awk -F: '{print $2}' | sed 's/[\" ]//g')
      line=$(grep -E "^\\s{8}user:" -m1 "$CONFIG_FILE" || true); [[ -n "$line" ]] && DB_USER=$(echo "$line" | awk -F: '{print $2}' | sed 's/[\" ]//g')
      line=$(grep -E "^\\s{8}passwd:" -m1 "$CONFIG_FILE" || true); [[ -n "$line" ]] && DB_PASS=$(echo "$line" | awk -F: '{print $2}' | sed 's/[\" ]//g')
      line=$(grep -E "^\\s{8}dbname:" -m1 "$CONFIG_FILE" || true); [[ -n "$line" ]] && DB_NAME=$(echo "$line" | awk -F: '{print $2}' | sed 's/[\" ]//g')
    fi
  fi

  if [[ -z "$DB_HOST" || -z "$DB_PORT" || -z "$DB_USER" || -z "$DB_NAME" ]]; then
    echo "数据库连接信息不完整，请通过 --host/--port/--user/--pass/--name 指定，或在 $CONFIG_FILE 中配置。" >&2
    exit 1
  fi
}

mysql_base() {
  mysql -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -p"$DB_PASS" --default-character-set=utf8mb4 -N -s "$@"
}

mysql_db() {
  mysql_base -D "$DB_NAME" "$@"
}

ensure_database() {
  mysql_base -e "CREATE DATABASE IF NOT EXISTS \`$DB_NAME\` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;" >/dev/null
}

ensure_schema_table() {
  mysql_db -e '
    CREATE TABLE IF NOT EXISTS `schema_migrations` (
      `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
      `filename` VARCHAR(255) NOT NULL UNIQUE,
      `checksum` CHAR(64) NOT NULL,
      `applied_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
      PRIMARY KEY (`id`)
    ) ENGINE=InnoDB;
  ' >/dev/null
}

sha256() {
  # 输出文件的 sha256 值
  sha256sum "$1" | awk '{print $1}'
}

list_migrations() {
  ensure_database
  ensure_schema_table
  local files applied chk a_time
  mapfile -t files < <(ls -1 "$MIGRATIONS_DIR"/*.sql 2>/dev/null | sort)
  if [[ ${#files[@]} -eq 0 ]]; then
    echo "(无迁移文件)"
    return 0
  fi
  printf "%-35s | %-19s | %s\n" "迁移文件" "应用时间" "状态"
  printf -- "%.0s-" {1..80}; echo
  for f in "${files[@]}"; do
    local name
    name=$(basename "$f")
    local row applied chk a_time
    row=$(mysql_db -e "SELECT 1, checksum, DATE_FORMAT(applied_at, '%Y-%m-%d %H:%i:%s') FROM schema_migrations WHERE filename='${name}' LIMIT 1;" 2>/dev/null || true)
    applied=$(echo "$row" | awk '{print $1}')
    chk=$(echo "$row" | awk '{print $2}')
    a_time=$(echo "$row" | awk '{print $3}')
    if [[ "$applied" == "1" ]]; then
      printf "%-35s | %-19s | applied\n" "$name" "${a_time:-}"
    else
      printf "%-35s | %-19s | pending\n" "$name" "-"
    fi
  done
}

status_migrations() {
  ensure_database
  ensure_schema_table
  local total applied pending last
  total=$(ls -1 "$MIGRATIONS_DIR"/*.sql 2>/dev/null | wc -l | tr -d ' ')
  applied=$(mysql_db -e 'SELECT COUNT(1) FROM schema_migrations;' | tr -d ' ')
  pending=$(( total - applied ))
  last=$(mysql_db -e "SELECT filename FROM schema_migrations ORDER BY id DESC LIMIT 1;" || true)
  echo "Total: $total, Applied: $applied, Pending: $pending"
  if [[ -n "$last" ]]; then
    echo "Last: $last"
  fi
}

apply_migrations() {
  ensure_database
  ensure_schema_table
  mapfile -t files < <(ls -1 "$MIGRATIONS_DIR"/*.sql 2>/dev/null | sort)
  if [[ ${#files[@]} -eq 0 ]]; then
    echo "没有需要执行的迁移文件。"
    return 0
  fi
  for f in "${files[@]}"; do
    local name checksum is_applied existing_chk
    name=$(basename "$f")
    checksum=$(sha256 "$f")
    read -r is_applied existing_chk < <(mysql_db -e "SELECT 1, checksum FROM schema_migrations WHERE filename='${name}' LIMIT 1;" || true)
    if [[ "$is_applied" == "1" ]]; then
      if [[ "$existing_chk" != "$checksum" ]]; then
        echo "警告: 迁移文件 ${name} 内容已改变（校验和不一致）。为确保一致性，已跳过。" >&2
      else
        echo "已应用，跳过: $name"
      fi
      continue
    fi
    echo "应用迁移: $name"
    # 对 001_init_db.sql 这类文件包含 CREATE DATABASE/USE 的情况，直接执行也安全
    if ! mysql_db < "$f"; then
      echo "执行失败: $name" >&2
      exit 1
    fi
    mysql_db -e "INSERT INTO schema_migrations(filename, checksum) VALUES ('${name}', '${checksum}');" >/dev/null
  done
  echo "迁移完成。"
}

main() {
  case "$CMD" in
    list|status|apply) ;;
    *) usage; exit 1;;
  esac
  parse_args "$@"
  read_config_defaults
  case "$CMD" in
    list) list_migrations ;;
    status) status_migrations ;;
    apply) apply_migrations ;;
  esac
}

main "$@"
