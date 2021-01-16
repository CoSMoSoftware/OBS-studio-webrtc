package io.cosmosoftware.kite.obs.steps;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.TestCoordinator;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.steps.TestStep;

public class QuitApplicationStep extends TestStep {

  private final MainPage mainPage;
  private final TestCoordinator coordinator;

  public QuitApplicationStep(Runner runner, TestCoordinator coordinator) {
    super(runner);
    this.setNeverSkip(true);
    this.mainPage = new MainPage(runner);
    this.coordinator = coordinator;
  }

  @Override
  protected void step() throws KiteTestException {
    if (!coordinator.hasObsCrashed()) {
      mainPage.quitApplication();
    } else {
      logger.info("OBS crashed, quit step is skipped");
    }
  }

  @Override
  public String stepDescription() {
    return "Quit the application";
  }
}
