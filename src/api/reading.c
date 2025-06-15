const char *reading_table = "reading";
const char *reading_schema = "create table reading ("
														 "id blob primary key, "
														 "temperature real not null, "
														 "humidity real not null, "
														 "captured_at timestamp not null, "
														 "uplink_id blob not null, "
														 "foreign key (uplink_id) references uplink(id) on delete cascade"
														 ")";
