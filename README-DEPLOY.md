# PSS LifeLink — public deployment package

This package contains a Linux/Docker C++ web server using the MySQL C client, your existing HTML frontend, and your supplied SQL backup. The search features remain Hospital ID, City, Organ, and Blood Group and query the same `hospital`, `organ`, and `blood_bank` tables.

## Important limits
I cannot publish a live public URL without access to your hosting/GitHub account. You must sign in and create the services described below. Hosting providers can charge; check the current plan/pricing before confirming. Do not put real database passwords in source code or upload them to GitHub.

## Recommended hosting architecture
- Railway service A: MySQL database
- Railway service B: C++ web service built from this Dockerfile
- Railway-generated public domain for service B
- Database should remain private to the project; do not expose MySQL publicly unless absolutely necessary.

Railway docs: https://docs.railway.com/services and https://docs.railway.com/databases/mysql

## Publish steps
1. Create/sign in to GitHub and create a **private** repository named `pss-lifelink`.
2. Upload the files in this folder to the repository root: `Dockerfile`, `server.cpp`, `PSS-LifeLink-Website.html`. Keep `PSS backup.sql` private and don't upload if you don't need it in the repo; it contains sample contact information.
3. Sign in to Railway and create a new project. Add a MySQL database service. Check its Variables tab for `MYSQLHOST`, `MYSQLPORT`, `MYSQLUSER`, `MYSQLPASSWORD`, `MYSQLDATABASE`.
4. Import the provided `PSS backup.sql` into that Railway database using MySQL Workbench. Connect with the host/port/user/password supplied by Railway (the provider may require enabling a TCP proxy for external Workbench access). Select the database name shown in `MYSQLDATABASE` before executing the dump. Verify all three tables exist: `hospital`, `organ`, `blood_bank`.
5. Add a service from the GitHub repository. Railway will build from the root `Dockerfile`.
6. In the C++ service's Variables tab, add/reference these values from the MySQL service: `MYSQLHOST`, `MYSQLPORT`, `MYSQLUSER`, `MYSQLPASSWORD`, `MYSQLDATABASE`. `PORT` is supplied by Railway; the server reads it automatically.
7. Deploy. Check deployment logs for `PSS LifeLink running on port ...`.
8. In the service's Settings/Networking, generate a public domain. Open that HTTPS URL in Chrome on your phone/laptop and test `Bilaspur`, Hospital ID `101`, `Kidney`, and `O+`.

## If import complains about schema
The SQL dump does not select a database with a `USE` statement. In Workbench, first select the Railway database shown in `MYSQLDATABASE`, then execute the dump. Don't run the dump against a database containing unrelated tables; it contains `DROP TABLE IF EXISTS` for these three table names.

## Local build note
This Dockerfile is for a Linux container. The old Windows `server.cpp` is not used by this Dockerfile; the new Linux server preserves the same four search options and table-level data model but uses a Linux socket listener and MySQL client library. Keep `Original-console-source-backup.txt` as the original submitted console code.

## Safety
This is a college demonstration directory, not a real-time medical inventory. The sample availability/status data may be stale or fictional. Confirm with hospitals before acting. Do not store patient records. Restrict database credentials, use private repository, HTTPS domain, and rotate any password previously embedded in the original source.
