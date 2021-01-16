package io.cosmosoftware.kite.obs.checks;

import static io.cosmosoftware.kite.entities.Timeouts.DEFAULT_TIMEOUT;
import static io.cosmosoftware.kite.entities.Timeouts.FIVE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.ONE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.TEN_SECOND_INTERVAL_IN_SECONDS;
import static io.cosmosoftware.kite.util.TestUtils.waitAround;
import static io.cosmosoftware.kite.util.WebDriverUtils.poke;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.TestCoordinator;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.steps.GetCpuUsageStep;
import io.cosmosoftware.kite.obs.steps.GetStatsStep;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.StepPhase;
import io.cosmosoftware.kite.steps.TestStep;

public class MonitoringCheck extends TestStep {

  private final Runner runner;
  private final TestCoordinator coordinator;
  private final int duration;

  public MonitoringCheck(Runner runner,
      TestCoordinator coordinator, int duration) {
    super(runner);
    this.runner = runner;
    this.coordinator = coordinator;
    this.duration = duration;
    this.setOptional(true);
  }

  @Override
  protected void step() throws KiteTestException {
    this.waitForStream();

    for (int wait = 0; wait < this.duration - FIVE_SECOND_INTERVAL; wait += FIVE_SECOND_INTERVAL) {
      long start = System.currentTimeMillis();
      GetCpuUsageStep cpuUsageStep = new GetCpuUsageStep(this.runner);
      cpuUsageStep.setOptional(true);
      cpuUsageStep.setIgnoreBroken(true);
      if (!this.coordinator.isStreaming()) {
        break;
      }
      cpuUsageStep.processTestStep(StepPhase.DEFAULT, this.report, false);
      GetStatsStep getStatsStep = new GetStatsStep(this.runner, null, new MainPage(runner));
      getStatsStep.setIgnoreBroken(true);
      getStatsStep.processTestStep(StepPhase.DEFAULT, this.report, false);
      waitWithInteraction((int) (FIVE_SECOND_INTERVAL - (System.currentTimeMillis() - start)));
      poke(this.webDriver);
    }
  }

  private void waitWithInteraction(int duration) {
    for (int wait = 0; wait < duration; wait += ONE_SECOND_INTERVAL) {
      poke(this.webDriver); // so that browser doesn't time out
      waitAround(ONE_SECOND_INTERVAL);
    }
  }

  private void waitForStream() throws KiteTestException {
    logger.info("Waiting for stream confirmation");
    for (int wait = 0; wait < DEFAULT_TIMEOUT + this.coordinator.getStreamDuration();
        wait += ONE_SECOND_INTERVAL) {
      if (coordinator.hasObsCrashed()) {
        throw new KiteTestException("OBS has crashed while waiting for stream", Status.FAILED);
      }
      if (!coordinator.isStreaming()) {
        poke(this.webDriver); // so that browser doesn't time out
        waitAround(ONE_SECOND_INTERVAL);
      } else {
        logger.info("Streaming confirmed");
        waitAround(TEN_SECOND_INTERVAL_IN_SECONDS);
        return;
      }
    }
    this.setOptional(false);
    throw new KiteTestException("Could not get stream confirmation after 60s", Status.FAILED);
  }

  @Override
  public String stepDescription() {
    return "Retrieving cpu usage (in %) from node's system and stats from OBS periodically";
  }

}
