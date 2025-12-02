#ifndef ENUMS_ENUMS_H
#define ENUMS_ENUMS_H

namespace Enums {
    /// Gender a Pokemon can have
    /// Provided to function for Encounter template values
    enum class Gender
    {
        Male,
        Female,
        Genderless,
        Random = Genderless,
    };
}

#endif