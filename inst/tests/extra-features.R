test_that("logLik() is correct", {
  n <- 50
  group <- rep(0:4,5:1)
  p <- length(group)
  X <- matrix(rnorm(n*p),ncol=p)
  y <- rnorm(n)
  yy <- runif(n) > .5
  fit.mle <- lm(y~X)
  fit <- grpreg(X, y, group, penalty="grLasso", lambda.min=0)
  expect_that(logLik(fit)[100], equals(logLik(fit.mle)[1], check.attributes=FALSE, tol=.001))
  expect_that(AIC(fit)[100], equals(AIC(fit.mle), check.attributes=FALSE, tol=.001))
  fit <- grpreg(X, y, group, penalty="gMCP", lambda.min=0)
  expect_that(logLik(fit)[100], equals(logLik(fit.mle)[1], check.attributes=FALSE, tol=.001))
  expect_that(AIC(fit)[100], equals(AIC(fit.mle), check.attributes=FALSE, tol=.001))
  fit.mle <- glm(yy~X, family="binomial")
  fit <- grpreg(X, yy, group, penalty="grLasso", lambda.min=0, family="binomial")
  expect_that(logLik(fit)[100], equals(logLik(fit.mle)[1], check.attributes=FALSE, tol=.001))
  expect_that(AIC(fit)[100], equals(AIC(fit.mle), check.attributes=FALSE, , tol=.001))
  fit <- grpreg(X, yy, group, penalty="gMCP", lambda.min=0, family="binomial")
  expect_that(logLik(fit)[100], equals(logLik(fit.mle)[1], check.attributes=FALSE, tol=.001))  
  expect_that(AIC(fit)[100], equals(AIC(fit.mle), check.attributes=FALSE, tol=.001))
})

test_that("grpreg handles constant columns", {
  n <- 50
  group <- rep(0:3,4:1)
  p <- length(group)
  X <- matrix(rnorm(n*p),ncol=p)
  X[,group==2] <- 0
  y <- rnorm(n)
  yy <- y > 0
  fit <- grpreg(X, y, group, penalty="grLasso")
  fit <- grpreg(X, y, group, penalty="gMCP")
  fit <- grpreg(X, yy, group, penalty="grLasso", family="binomial")
  fit <- grpreg(X, yy, group, penalty="gMCP", family="binomial")
})

test_that("cv.grpreg() seems to work", {
  n <- 50
  group <- rep(1:4,1:4)
  p <- length(group)
  X <- matrix(rnorm(n*p),ncol=p)
  b <- rnorm(p, sd=2)
  b[abs(b) < 1] <- 0
  y <- rnorm(n, mean=X%*%b)
  yy <- y > .5
  
  par(mfrow=c(2,2))
  require(glmnet)
  cvfit <- cv.glmnet(X, y)
  plot(cvfit)
  cvfit <- cv.grpreg(X, y, group)
  plot(cvfit)
  cvfit <- cv.glmnet(X, yy, family="binomial")
  plot(cvfit)
  cvfit <- cv.grpreg(X, yy, group, family="binomial")
  plot(cvfit)
})  

test_that("group.multiplier works", {
  n <- 50
  p <- 10
  X <- matrix(rnorm(n*p),ncol=p)
  y <- rnorm(n)
  group <- rep(0:3,1:4)
  gm <- 1:3
  par(mfrow=c(3,2))
  plot(fit <- grpreg(X, y, group, penalty="gMCP", lambda.min=0, group.multiplier=gm), main=fit$penalty)
  plot(fit <- gBridge(X, y, group, lambda.min=0, group.multiplier=gm), main=fit$penalty)
  plot(fit <- grpreg(X, y, group, penalty="grLasso", lambda.min=0, group.multiplier=gm), main=fit$penalty)
  plot(fit <- grpreg(X, y, group, penalty="grMCP", lambda.min=0, group.multiplier=gm), main=fit$penalty)
  plot(fit <- grpreg(X, y, group, penalty="grSCAD", lambda.min=0, group.multiplier=gm), main=fit$penalty)
})