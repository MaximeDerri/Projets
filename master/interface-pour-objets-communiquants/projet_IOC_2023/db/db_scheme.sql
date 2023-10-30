-- SQLite DATABASE for IOC project


-- client identifier
CREATE TABLE client
(
  id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  name TEXT UNIQUE NOT NULL CHECK (name <> '')
);


-- push button
CREATE TABLE pb_sample
(
  id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  id_client INTEGER NOT NULL UNIQUE,
  counter INTEGER NOT NULL CHECK (counter >= 0),
  date_sample DATETIME NOT NULL,
  FOREIGN KEY (id_client) REFERENCES client(id) ON DELETE CASCADE
);


-- led
CREATE TABLE led_sample
(
  id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  id_client INTEGER NOT NULL UNIQUE,
  state BOOLEAN NOT NULL,
  date_sample DATETIME NOT NULL,
  FOREIGN KEY (id_client) REFERENCES client(id) ON DELETE CASCADE
);


-- photoresistor
CREATE TABLE pres_sample
(
  id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  id_client INTEGER NOT NULL,
  val INTEGER NOT NULL CHECK (val >= 0 AND val <= 100),
  date_sample DATETIME NOT NULL,
  FOREIGN KEY (id_client) REFERENCES client(id) ON DELETE CASCADE
);


-- trigger
CREATE TRIGGER monitor_samples 
AFTER INSERT ON pres_sample 
WHEN 51 < (SELECT COUNT(*) FROM pres_sample WHERE pres_sample.id_client=NEW.id_client) 
BEGIN 
  DELETE FROM pres_sample WHERE pres_sample.id IN 
    (SELECT MIN(pres_sample.id) FROM pres_sample WHERE pres_sample.id_client=NEW.id_client); 
END;
