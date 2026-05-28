#ifndef CYNIDE_STAGE_H
#define CYNIDE_STAGE_H

#include <string>

/**
 * @brief Base interface representing a compiler stage.
 *
 * Each stage of the compilation pipeline inherits from this class to expose
 * a unified error handling interface, enabling stages to report failure state
 * and associated error messages.
 */
class Stage {
public:
  virtual ~Stage() = default;

  /**
   * @brief Checks if this stage has encountered any compilation errors.
   * @return true if an error has occurred, false otherwise.
   */
  virtual bool hasError() const = 0;

  /**
   * @brief Retrieves the error message if the stage has failed.
   * @return A string description of the encountered error.
   */
  virtual std::string errorMessage() const = 0;
};

#endif // CYNIDE_STAGE_H