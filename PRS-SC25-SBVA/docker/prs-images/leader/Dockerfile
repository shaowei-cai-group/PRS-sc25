FROM satcomp-prs-sbva:common AS builder
USER root

FROM satcomp-infrastructure:leader

WORKDIR /

COPY --from=builder /PRS/build/prs /PRS

COPY --chown=ecs-user /docker/prs-images/leader/init_solver.sh /competition
COPY --chown=ecs-user /docker/prs-images/leader/solver /competition

USER ecs-user

RUN chmod +x /competition/init_solver.sh
RUN chmod +x /competition/solver