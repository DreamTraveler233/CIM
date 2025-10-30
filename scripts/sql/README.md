# 数据库迁移使用说明

本仓库使用简单的 Bash 迁移脚本来管理数据库结构变更。

- 迁移文件目录：`migrations/`（按数字前缀排序执行）
- 迁移状态表：`schema_migrations`（自动创建于目标库中）
- 默认数据库连接：从 `bin/config/system.yaml` 的 `mysql.dbs.default` 读取

## 可用命令

- 列出迁移列表及状态：

  scripts/sql/migrate.sh list

- 查看摘要状态：

  scripts/sql/migrate.sh status

- 应用所有未执行的迁移：

  scripts/sql/migrate.sh apply

如需显式指定连接参数，可追加：`--host --port --user --pass --name`

## 迁移文件规范

- 文件名形如：`NNN_description.sql`，以递增编号排序执行。
- 建议使用 `CREATE TABLE IF NOT EXISTS` 以提高幂等性。
- 不必在每个文件中书写 `USE <db>;`，脚本会通过 `-D <db>` 指定默认库。
- 首次建库语句置于 `001_init_db.sql`，包含 `CREATE DATABASE IF NOT EXISTS`。

## 小贴士

- 若修改了已应用迁移文件，脚本会检测到校验和变化并提示不一致；请新增增量文件而非直接修改历史迁移。
- 执行时会自动创建数据库与 `schema_migrations` 表，无需手动处理。
