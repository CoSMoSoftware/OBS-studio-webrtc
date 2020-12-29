package io.cosmosoftware.kite.obs.steps;

import static io.cosmosoftware.kite.entities.Timeouts.FIVE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.ONE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.TEN_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.THREE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.util.ReportUtils.saveScreenshotPNG;
import static io.cosmosoftware.kite.util.TestUtils.waitAround;
import static io.cosmosoftware.kite.util.WebDriverUtils.poke;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.TestCoordinator;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestStep;
import org.openqa.selenium.Rectangle;

public class StartStreamingStep extends TestStep {

  private final int streamDuration;
  private final MainPage mainPage;
  private final Runner runner;
  private final TestCoordinator coordinator;

  public StartStreamingStep(Runner runner, int streamDuration, TestCoordinator coordinator) {
    super(runner);
    this.setScreenShotOnFailure(true);
    this.streamDuration = streamDuration;
    this.mainPage = new MainPage(runner);
    this.runner = runner;
    this.coordinator = coordinator;
  }

  @Override
  protected void step() throws KiteTestException {
    try {
      this.mainPage.startStreaming();
      this.waitForStopStreamingButton();
      waitAround(FIVE_SECOND_INTERVAL);
      this.coordinator.setStreaming(true);
    } catch (Exception e) {
      this.coordinator.setStreaming(false);
      this.coordinator.setObsCrashed(true);
      throw new KiteTestException("The OBS could not start streaming, probably crashed",
          Status.FAILED);
    }
    if (coordinator.isLongetiviteTest()) {
//      for (int wait = 0; wait < this.streamDuration - FIVE_SECOND_INTERVAL; wait += FIVE_SECOND_INTERVAL) {
//        long start = System.currentTimeMillis();
//        if (mainPage.crashed()) {
//          this.coordinator.setStreaming(false);
//          this.coordinator.setObsCrashed(true);
//          throw new KiteTestException("The OBS client has closed down, probably crashed", Status.FAILED);
//        }
//        GetCpuUsageStep cpuUsageStep = new GetCpuUsageStep(this.runner);
//        cpuUsageStep.setOptional(true);
//        if (!this.coordinator.isStreaming()) {
//          break;
//        }
//        cpuUsageStep.processTestStep(StepPhase.DEFAULT, this.report, false);
//        waitWithInteraction((int) (FIVE_SECOND_INTERVAL - (System.currentTimeMillis() - start)));
//
//      }
    } else {
      for (int wait = 0; wait < streamDuration; wait += ONE_SECOND_INTERVAL) {
        waitAround(ONE_SECOND_INTERVAL / 3);
        if (mainPage.crashed()) {
          this.coordinator.setStreaming(false);
          this.coordinator.setObsCrashed(true);
          throw new KiteTestException("The OBS client has closed down, probably crashed",
              Status.FAILED);
        }
      }
    }
  }

  private void waitWithInteraction(int duration) {
    for (int wait = 0; wait < duration; wait += ONE_SECOND_INTERVAL) {
      //this.webDriver.getTitle();
      poke(this.webDriver); // so that browser doesn't time out
      waitAround(ONE_SECOND_INTERVAL);
    }
  }

  private void waitForStopStreamingButton() throws KiteTestException {
    for (int wait = 0; wait < TEN_SECOND_INTERVAL; wait += ONE_SECOND_INTERVAL) {
      if (mainPage.getStreamingButtonStatus().contains("Stop")) {
        return;
      }
      waitAround(ONE_SECOND_INTERVAL);
    }
    throw new KiteTestException("The OBS client has closed down, probably crashed", Status.FAILED);
  }

  @Override
  public void execute() {
    try {
      logger.info(this.getStepPhase().getShortName() + stepDescription());
      step();
    } catch (Exception e) {
      String screenshotName = "error_screenshot_" + this.getName();
      try {
        Rectangle rect = new Rectangle(0, 0, 1000, 1500);
        waitAround(THREE_SECOND_INTERVAL);
        reporter
            .screenshotAttachment(this.report, screenshotName, saveScreenshotPNG(webDriver, rect));
      } catch (KiteTestException ex) {
        logger.warn("Could not attach screenshot to error of step: " + stepDescription());
      }
      reporter.processException(this.report, e, false);
    }
  }

  @Override
  public String stepDescription() {
    return "Start streaming for " + streamDuration / 1000 + "s";
  }
}
