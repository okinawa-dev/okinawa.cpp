#include "okinawa/core/object.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

// What a node may decide about its own subtree, and which pass its
// geometry belongs to.
//
// Both questions used to have one answer for everybody: draw() always
// recursed, and the scene sorted opaque from blended by asking each
// ROOT -- which answered for its children too. Neither mattered while
// every object in the world was a root; both matter the moment one of
// them stands for a region and owns what is in it.

namespace {

  // A node that records what it was asked to do, and can refuse.
  class Spy : public OkObject {
  public:
    Spy(const std::string &name, std::vector<std::string> *log)
        : OkObject(name) {
      this->log      = log;
      this->drawing  = true;
      this->stepping = true;
      this->blended  = false;
    }

    bool drawing;
    bool stepping;
    bool blended;

    bool shouldDraw() const override {
      return drawing;
    }
    bool shouldStep(float) const override {
      return stepping;
    }
    bool isBlended() const override {
      return blended;
    }

  protected:
    void drawSelf() override {
      log->push_back("draw:" + getName());
    }
    void stepSelf(float) override {
      log->push_back("step:" + getName());
    }
    void updateTransformSelf() override {}

  private:
    std::vector<std::string> *log;
  };

  bool contains(const std::vector<std::string> &log, const std::string &what) {
    for (size_t i = 0; i < log.size(); i++) {
      if (log[i] == what) {
        return true;
      }
    }
    return false;
  }

}  // namespace

TEST_CASE("A node that says it is not drawn takes its children with it",
          "[object]") {
  std::vector<std::string> log;
  Spy                      parent("parent", &log);
  auto                    *child = new Spy("child", &log);
  parent.attach(child);

  parent.draw();
  REQUIRE(contains(log, "draw:parent"));
  REQUIRE(contains(log, "draw:child"));

  log.clear();
  parent.drawing = false;
  parent.draw();
  // One question, one subtree: the point is that the child costs
  // nothing, not that the parent is quiet.
  REQUIRE(log.empty());
}

TEST_CASE("The same holds for the step", "[object]") {
  std::vector<std::string> log;
  Spy                      parent("parent", &log);
  auto                    *child = new Spy("child", &log);
  parent.attach(child);

  parent.step(0.016f);
  REQUIRE(contains(log, "step:child"));

  log.clear();
  parent.stepping = false;
  parent.step(0.016f);
  REQUIRE(log.empty());
}

TEST_CASE("An object is drawn in its own pass, not its parent's", "[object]") {
  std::vector<std::string> log;
  Spy                      parent("wall", &log);  // opaque
  auto                    *halo = new Spy("halo", &log);
  halo->blended                 = true;
  parent.attach(halo);

  parent.drawPass(false);
  REQUIRE(contains(log, "draw:wall"));
  REQUIRE_FALSE(contains(log, "draw:halo"));

  log.clear();
  parent.drawPass(true);
  // The blended pass reaches the halo THROUGH its opaque parent: the
  // traversal is not stopped by a parent that belongs to the other
  // pass, or a glow inside a building would never be drawn.
  REQUIRE(contains(log, "draw:halo"));
  REQUIRE_FALSE(contains(log, "draw:wall"));
}

TEST_CASE("Drawing a subtree on its own does both passes, opaque first",
          "[object]") {
  std::vector<std::string> log;
  Spy                      parent("wall", &log);
  auto                    *halo = new Spy("halo", &log);
  halo->blended                 = true;
  parent.attach(halo);

  parent.draw();
  REQUIRE(log.size() == 2);
  REQUIRE(log[0] == "draw:wall");
  REQUIRE(log[1] == "draw:halo");
}
