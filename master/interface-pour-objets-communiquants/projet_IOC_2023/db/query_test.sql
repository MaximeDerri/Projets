-- SQLite query tests


INSERT INTO client (name) VALUES ('c1');
INSERT INTO client (name) VALUES ('c2');
INSERT INTO client (name) VALUES ('c3');
INSERT INTO client (name) VALUES ('c4');
INSERT INTO client (name) VALUES ('c5');

INSERT INTO led_sample (id_client, state, date_sample) VALUES (2, 0, DATETIME());
INSERT INTO pres_sample (id_client, val, date_sample) VALUES (2, 10, DATETIME());
INSERT INTO pres_sample (id_client, val, date_sample) VALUES (2, 20, DATETIME());
INSERT INTO pres_sample (id_client, val, date_sample) VALUES (2, 30, DATETIME());
INSERT INTO pres_sample (id_client, val, date_sample) VALUES (2, 40, DATETIME());


INSERT INTO pb_sample (id_client, counter, date_sample) VALUES (1, 0, DATETIME());
INSERT INTO pb_sample (id_client, counter, date_sample) VALUES (2, 0, DATETIME());
SELECT * FROM client, pb_sample WHERE client.id=pb_sample.id;


UPDATE pb_sample SET counter=20 WHERE id=1;
SELECT * FROM client, pb_sample WHERE client.id=pb_sample.id;


INSERT INTO pres_sample (id_client, val, date_sample) VALUES (1, 10, DATETIME());
INSERT INTO pres_sample (id_client, val, date_sample) VALUES (1, 20, DATETIME());
INSERT INTO pres_sample (id_client, val, date_sample) VALUES (1, 30, DATETIME());
INSERT INTO pres_sample (id_client, val, date_sample) VALUES (1, 40, DATETIME());
INSERT INTO pres_sample (id_client, val, date_sample) VALUES (1, 50, DATETIME());
INSERT INTO pres_sample (id_client, val, date_sample) VALUES (1, 60, DATETIME());
INSERT INTO pres_sample (id_client, val, date_sample) VALUES (1, 70, DATETIME());
SELECT * FROM pres_sample;
SELECT COUNT(*) AS nb_sample_c1 FROM pres_sample WHERE pres_sample.id_client IN (SELECT id FROM client WHERE name='c1');


DELETE FROM client WHERE name='c1';
SELECT COUNT(*) AS nb_empty FROM pres_sample WHERE pres_sample.id_client IN (SELECT id FROM client WHERE name='c1');
