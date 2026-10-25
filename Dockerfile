FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends g++ make default-libmysqlclient-dev ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY server.cpp ./server.cpp
COPY PSS-LifeLink-Website.html ./PSS-LifeLink-Website.html
RUN g++ -std=c++17 -O2 -Wall server.cpp -o server -lmysqlclient
ENV PORT=8080
EXPOSE 8080
CMD ["/app/server"]
