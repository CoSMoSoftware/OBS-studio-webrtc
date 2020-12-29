package io.cosmosoftware.kite.obs.steps;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.TestCoordinator;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.steps.TestStep;

public class StopStreamingStep extends TestStep {

  private final MainPage mainPage;
  private final TestCoordinator coordinator;

  public StopStreamingStep(Runner runner, TestCoordinator coordinator) {
    super(runner);
    this.mainPage = new MainPage(runner);
    this.coordinator = coordinator;
  }

  @Override
  protected void step() throws KiteTestException {
    this.mainPage.stopStreaming();
    this.coordinator.setStreaming(false);
  }

  @Override
  public String stepDescription() {
    return "Stop current stream";
  }
}
