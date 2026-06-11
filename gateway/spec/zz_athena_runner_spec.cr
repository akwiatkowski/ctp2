require "./spec_helper"

# Executes every ASPEC::TestCase (PagesTest, ApiTest) — must compile AFTER
# the struct definitions, hence the zz_ filename (crystal spec loads files
# alphabetically).
Athena::Spec.run_all
