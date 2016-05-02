/***********************************************************************
/
/ Example executable using libgrackle
/
/
/ Copyright (c) 2013, Enzo/Grackle Development Team.
/
/ Distributed under the terms of the Enzo Public Licence.
/
/ The full license is in the file LICENSE, distributed with this 
/ software.
************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <grackle.h>

#define mh     1.67262171e-24   
#define kboltz 1.3806504e-16

int main(int argc, char *argv[])
{

  /*********************************************************************
  / Initial setup of units and chemistry objects.
  / This should be done at simulation start.
  *********************************************************************/

  // Enable output
  grackle_verbose = 1;

  // First, set up the units system.
  // These are conversions from code units to cgs.
  code_units my_units;
  my_units.comoving_coordinates = 0; // 1 if cosmological sim, 0 if not
  my_units.density_units = 1.67e-24;
  my_units.length_units = 1.0;
  my_units.time_units = 1.0e12;
  my_units.velocity_units = my_units.length_units / my_units.time_units;
  my_units.a_units = 1.0; // units for the expansion factor

  // Second, create a chemistry object for parameters and rate data.
  if (set_default_chemistry_parameters() == 0) {
    fprintf(stderr, "Error in set_default_chemistry_parameters.\n");
    return 0;
  }

  // Set parameter values for chemistry.
  grackle_data.use_grackle = 1;            // chemistry on
  grackle_data.with_radiative_cooling = 1; // cooling on
  grackle_data.primordial_chemistry = 3;   // molecular network with H, He, D
  grackle_data.metal_cooling = 1;          // metal cooling on
  grackle_data.UVbackground = 1;           // UV background on
  grackle_data.grackle_data_file = "../../input/CloudyData_UVB=HM2012.h5"; // data file

  // Set initial expansion factor (for internal units).
  // Set expansion factor to 1 for non-cosmological simulation.
  double initial_redshift = 0.;
  double a_value = 1. / (1. + initial_redshift);

  // Finally, initialize the chemistry object.
  if (initialize_chemistry_data(&my_units, a_value) == 0) {
    fprintf(stderr, "Error in initialize_chemistry_data.\n");
    return 0;
  }

  double tiny_number = 1.e-20;

  // Create struct for storing grackle field data
  grackle_field_data my_fields;

  // Set grid dimension and size.
  // grid_start and grid_end are used to ignore ghost zones.
  int field_size = 1;
  my_fields.grid_rank = 3;
  my_fields.grid_dimension = malloc(field_size * sizeof(int));
  my_fields.grid_start = malloc(field_size * sizeof(int));
  my_fields.grid_end = malloc(field_size * sizeof(int));

  int i;
  for (i = 0;i < 3;i++) {
    my_fields.grid_dimension[i] = 1; // the active dimension not including ghost zones.
    my_fields.grid_start[i] = 0;
    my_fields.grid_end[i] = 0;
  }
  my_fields.grid_dimension[0] = field_size;
  my_fields.grid_end[0] = field_size - 1;

  my_fields.density         = malloc(field_size * sizeof(gr_float));
  my_fields.internal_energy = malloc(field_size * sizeof(gr_float));
  my_fields.x_velocity      = malloc(field_size * sizeof(gr_float));
  my_fields.y_velocity      = malloc(field_size * sizeof(gr_float));
  my_fields.z_velocity      = malloc(field_size * sizeof(gr_float));
  // for primordial_chemistry >= 1
  my_fields.HI_density      = malloc(field_size * sizeof(gr_float));
  my_fields.HII_density     = malloc(field_size * sizeof(gr_float));
  my_fields.HeI_density     = malloc(field_size * sizeof(gr_float));
  my_fields.HeII_density    = malloc(field_size * sizeof(gr_float));
  my_fields.HeIII_density   = malloc(field_size * sizeof(gr_float));
  my_fields.e_density       = malloc(field_size * sizeof(gr_float));
  // for primordial_chemistry >= 2
  my_fields.HM_density      = malloc(field_size * sizeof(gr_float));
  my_fields.H2I_density     = malloc(field_size * sizeof(gr_float));
  my_fields.H2II_density    = malloc(field_size * sizeof(gr_float));
  // for primordial_chemistry >= 3
  my_fields.DI_density      = malloc(field_size * sizeof(gr_float));
  my_fields.DII_density     = malloc(field_size * sizeof(gr_float));
  my_fields.HDI_density     = malloc(field_size * sizeof(gr_float));
  // for metal_cooling = 1
  my_fields.metal_density   = malloc(field_size * sizeof(gr_float));

  // set constant heating rate terms (set as NULL pointers if not wanted)
  // volumetric heating rate (provide in units [erg s^-1 cm^-3])
  my_fields.volumetric_heating_rate = malloc(field_size * sizeof(gr_float));
  // specific heating rate (provide in units [egs s^-1 g^-1]
  my_fields.specific_heating_rate = malloc(field_size * sizeof(gr_float));

  // set temperature units
  double temperature_units =  mh * pow(my_units.a_units * 
                                         my_units.length_units /
                                         my_units.time_units, 2) / kboltz;

  for (i = 0;i < field_size;i++) {
    my_fields.density[i] = 1.0;
    my_fields.HI_density[i] = grackle_data.HydrogenFractionByMass * my_fields.density[i];
    my_fields.HII_density[i] = tiny_number * my_fields.density[i];
    my_fields.HM_density[i] = tiny_number * my_fields.density[i];
    my_fields.HeI_density[i] = (1.0 - grackle_data.HydrogenFractionByMass) *
      my_fields.density[i];
    my_fields.HeII_density[i] = tiny_number * my_fields.density[i];
    my_fields.HeIII_density[i] = tiny_number * my_fields.density[i];
    my_fields.H2I_density[i] = tiny_number * my_fields.density[i];
    my_fields.H2II_density[i] = tiny_number * my_fields.density[i];
    my_fields.DI_density[i] = 2.0 * 3.4e-5 * my_fields.density[i];
    my_fields.DII_density[i] = tiny_number * my_fields.density[i];
    my_fields.HDI_density[i] = tiny_number * my_fields.density[i];
    my_fields.e_density[i] = tiny_number * my_fields.density[i];
    // solar metallicity
    my_fields.metal_density[i] = grackle_data.SolarMetalFractionByMass *
      my_fields.density[i];

    my_fields.x_velocity[i] = 0.0;
    my_fields.y_velocity[i] = 0.0;
    my_fields.z_velocity[i] = 0.0;

    // initilize internal energy (here 1000 K for no reason)
    my_fields.internal_energy[i] = 1000. / temperature_units;

    my_fields.volumetric_heating_rate[i] = 0.0;
    my_fields.specific_heating_rate[i] = 0.0;
  }

  /*********************************************************************
  / Calling the chemistry solver
  / These routines can now be called during the simulation.
  *********************************************************************/

  // Evolving the chemistry.
  // some timestep
  double dt = 3.15e7 * 1e6 / my_units.time_units;

  if (solve_chemistry_new(&my_units, &my_fields,
                          a_value, dt) == 0) {
    fprintf(stderr, "Error in solve_chemistry.\n");
    return 0;
  }

  // Calculate cooling time.
  gr_float *cooling_time;
  cooling_time = malloc(field_size * sizeof(gr_float));
  if (calculate_cooling_time_new(&my_units, &my_fields,
                                 a_value, cooling_time) == 0) {
    fprintf(stderr, "Error in calculate_cooling_time.\n");
    return 0;
  }

  fprintf(stderr, "Cooling time = %g s.\n", cooling_time[0] *
          my_units.time_units);

  // Calculate temperature.
  gr_float *temperature;
  temperature = malloc(field_size * sizeof(gr_float));
  if (calculate_temperature_new(&my_units, &my_fields,
                                a_value, temperature) == 0) {
    fprintf(stderr, "Error in calculate_temperature.\n");
    return 0;
  }

  fprintf(stderr, "Temperature = %g K.\n", temperature[0]);

  // Calculate pressure.
  gr_float *pressure;
  pressure = malloc(field_size * sizeof(gr_float));
  if (calculate_pressure_new(&my_units, &my_fields,
                             a_value, pressure) == 0) {
    fprintf(stderr, "Error in calculate_pressure.\n");
    return 0;
  }

  fprintf(stderr, "Pressure = %g.\n", pressure[0]);

  // Calculate gamma.
  gr_float *gamma;
  gamma = malloc(field_size * sizeof(gr_float));
  if (calculate_gamma_new(&my_units, &my_fields,
                          a_value, gamma) == 0) {
    fprintf(stderr, "Error in calculate_gamma.\n");
    return 0;
  }

  fprintf(stderr, "gamma = %g.\n", gamma[0]);

  return 1;
}
