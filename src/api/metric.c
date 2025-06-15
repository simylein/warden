const char *metric_table = "metric";
const char *metric_schema = "create table metric ("
														"id blob primary key, "
														"photovoltaic real not null, "
														"battery real not null, "
														"captured_at timestamp not null, "
														"uplink_id blob not null unique, "
														"foreign key (uplink_id) references uplink(id) on delete cascade"
														")";
