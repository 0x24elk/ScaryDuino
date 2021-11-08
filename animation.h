class Animation {
 public:
  // Renders the next step in the animation.
  // pct is a value from [0,100] telling what state of
  // the animation to render where 0 is the beginning and
  // 100 is the completed animation.
  virtual void step(int pct) = 0;
};
