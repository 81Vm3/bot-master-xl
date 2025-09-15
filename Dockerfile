FROM ubuntu:22.04
LABEL authors="rain"

WORKDIR /app

EXPOSE 7071

COPY ./bin/BotMasterXL /app/BotMasterXL
COPY ./bin/data /app/data/

ENTRYPOINT ["/app/BotMasterXL"]