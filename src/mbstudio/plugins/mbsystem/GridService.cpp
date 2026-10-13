// Reading GMT grids (netCDF COARDS/CF "nf" grids written by mbgrid, mbmosaic
// and GMT, and the legacy GMT 1-D "cf" layout) directly with netCDF-C.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

#include <netcdf.h>

#include "MbioServices.h"

namespace mbstudio::mbsystem {

namespace {

struct NcFile {
  int id = -1;
  ~NcFile() {
    if (id >= 0)
      nc_close(id);
  }
};

std::string textAttribute(int ncid, int varid, const char *name) {
  size_t len = 0;
  nc_type type = NC_NAT;
  if (nc_inq_att(ncid, varid, name, &type, &len) != NC_NOERR || type != NC_CHAR)
    return std::string();
  std::string value(len, '\0');
  if (nc_get_att_text(ncid, varid, name, value.data()) != NC_NOERR)
    return std::string();
  while (!value.empty() && value.back() == '\0')
    value.pop_back();
  return value;
}

bool doubleAttribute(int ncid, int varid, const char *name, double &value) {
  size_t len = 0;
  nc_type type = NC_NAT;
  if (nc_inq_att(ncid, varid, name, &type, &len) != NC_NOERR || len < 1 || type == NC_CHAR)
    return false;
  std::vector<double> values(len);
  if (nc_get_att_double(ncid, varid, name, values.data()) != NC_NOERR)
    return false;
  value = values[0];
  return true;
}

bool readCoordinate(int ncid, int dimid, std::vector<double> &values, std::string &name, std::string &units) {
  char dimName[NC_MAX_NAME + 1];
  size_t len = 0;
  if (nc_inq_dim(ncid, dimid, dimName, &len) != NC_NOERR || len < 2)
    return false;
  name = dimName;
  values.assign(len, 0.0);
  int varid = -1;
  if (nc_inq_varid(ncid, dimName, &varid) == NC_NOERR &&
      nc_get_var_double(ncid, varid, values.data()) == NC_NOERR) {
    // GMT and mbgrid describe geographic axes by units ("degrees_east") or
    // by long_name ("Longitude"); keep both for isGeographic().
    units = textAttribute(ncid, varid, "units") + " " + textAttribute(ncid, varid, "long_name");
    return true;
  }
  for (size_t i = 0; i < len; i++)  // no coordinate variable: use indices
    values[i] = static_cast<double>(i);
  return true;
}

bool isGeographic(const std::string &name, const std::string &units) {
  std::string n = name;
  std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  std::string u = units;
  std::transform(u.begin(), u.end(), u.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return n == "lon" || n == "longitude" || u.find("degrees_east") != std::string::npos ||
         u.find("degree_east") != std::string::npos || u.find("longitude") != std::string::npos;
}

void finishGrid(Grid &grid) {
  bool any = false;
  for (float v : grid.z) {
    if (std::isnan(v))
      continue;
    if (!any) {
      grid.zmin = grid.zmax = v;
      any = true;
    }
    grid.zmin = std::min(grid.zmin, static_cast<double>(v));
    grid.zmax = std::max(grid.zmax, static_cast<double>(v));
  }
}

// GMT "nf" grids: a 2-D variable with 1-D coordinate variables.
bool loadCoards(int ncid, GridLoadResult &result) {
  int nvars = 0;
  nc_inq_nvars(ncid, &nvars);
  int zid = -1;
  for (int v = 0; v < nvars; v++) {
    int ndims = 0;
    nc_inq_varndims(ncid, v, &ndims);
    if (ndims != 2)
      continue;
    char name[NC_MAX_NAME + 1];
    nc_inq_varname(ncid, v, name);
    if (zid < 0 || std::strcmp(name, "z") == 0)
      zid = v;
  }
  if (zid < 0)
    return false;

  int dims[2];
  nc_inq_vardimid(ncid, zid, dims);
  Grid &grid = result.grid;
  std::vector<double> xs, ys;
  std::string xUnits, yUnits;
  if (!readCoordinate(ncid, dims[1], xs, grid.xName, xUnits) ||
      !readCoordinate(ncid, dims[0], ys, grid.yName, yUnits)) {
    result.error = "grid coordinates could not be read";
    return true;
  }
  grid.nx = static_cast<int>(xs.size());
  grid.ny = static_cast<int>(ys.size());
  grid.xmin = std::min(xs.front(), xs.back());
  grid.xmax = std::max(xs.front(), xs.back());
  grid.ymin = std::min(ys.front(), ys.back());
  grid.ymax = std::max(ys.front(), ys.back());
  grid.geographic = isGeographic(grid.xName, xUnits);
  char zname[NC_MAX_NAME + 1];
  nc_inq_varname(ncid, zid, zname);
  grid.zName = zname;
  grid.zUnits = textAttribute(ncid, zid, "units");
  grid.title = textAttribute(ncid, NC_GLOBAL, "title");

  std::vector<float> raw(static_cast<size_t>(grid.nx) * grid.ny);
  if (nc_get_var_float(ncid, zid, raw.data()) != NC_NOERR) {
    result.error = "grid values could not be read";
    return true;
  }
  double fill = std::numeric_limits<double>::quiet_NaN();
  double missing = std::numeric_limits<double>::quiet_NaN();
  doubleAttribute(ncid, zid, "_FillValue", fill);
  doubleAttribute(ncid, zid, "missing_value", missing);
  double scale = 1.0, offset = 0.0;
  doubleAttribute(ncid, zid, "scale_factor", scale);
  doubleAttribute(ncid, zid, "add_offset", offset);

  // Store north-up: row 0 = ymax.
  const bool yAscending = ys.back() > ys.front();
  const bool xAscending = xs.back() > xs.front();
  grid.z.assign(raw.size(), std::numeric_limits<float>::quiet_NaN());
  for (int iy = 0; iy < grid.ny; iy++) {
    const int srcRow = yAscending ? grid.ny - 1 - iy : iy;
    for (int ix = 0; ix < grid.nx; ix++) {
      const int srcCol = xAscending ? ix : grid.nx - 1 - ix;
      float v = raw[static_cast<size_t>(srcRow) * grid.nx + srcCol];
      if (std::isnan(v) || (!std::isnan(fill) && v == static_cast<float>(fill)) ||
          (!std::isnan(missing) && v == static_cast<float>(missing)))
        continue;
      grid.z[static_cast<size_t>(iy) * grid.nx + ix] = static_cast<float>(v * scale + offset);
    }
  }
  return true;
}

// Legacy GMT "cf" grids: x_range, y_range, z_range, spacing, dimension, z(xysize).
bool loadLegacy(int ncid, GridLoadResult &result) {
  int xr = -1, yr = -1, dimv = -1, zid = -1;
  if (nc_inq_varid(ncid, "x_range", &xr) != NC_NOERR || nc_inq_varid(ncid, "y_range", &yr) != NC_NOERR ||
      nc_inq_varid(ncid, "dimension", &dimv) != NC_NOERR || nc_inq_varid(ncid, "z", &zid) != NC_NOERR)
    return false;
  double xrange[2], yrange[2];
  int dimension[2];
  nc_get_var_double(ncid, xr, xrange);
  nc_get_var_double(ncid, yr, yrange);
  nc_get_var_int(ncid, dimv, dimension);
  Grid &grid = result.grid;
  grid.nx = dimension[0];
  grid.ny = dimension[1];
  grid.xmin = xrange[0];
  grid.xmax = xrange[1];
  grid.ymin = yrange[0];
  grid.ymax = yrange[1];
  grid.geographic = isGeographic("", textAttribute(ncid, xr, "units"));
  grid.title = textAttribute(ncid, NC_GLOBAL, "title");
  grid.zUnits = textAttribute(ncid, zid, "units");
  grid.z.assign(static_cast<size_t>(grid.nx) * grid.ny, 0.0f);
  if (grid.nx < 2 || grid.ny < 2 || nc_get_var_float(ncid, zid, grid.z.data()) != NC_NOERR) {
    result.error = "grid values could not be read";
    return true;
  }
  double fill = std::numeric_limits<double>::quiet_NaN();
  doubleAttribute(ncid, zid, "_FillValue", fill);
  for (float &v : grid.z)
    if (!std::isnan(fill) && v == static_cast<float>(fill))
      v = std::numeric_limits<float>::quiet_NaN();
  return true;  // legacy grids are stored north-up already
}

}  // namespace

GridLoadResult GridService::load(const std::string &path) const {
  GridLoadResult result;
  NcFile file;
  // GMT grid file names may carry a format suffix ("grid.grd=nf").
  std::string ncpath = path;
  const size_t eq = ncpath.rfind('=');
  if (eq != std::string::npos && eq + 3 >= ncpath.size())
    ncpath = ncpath.substr(0, eq);
  const int status = nc_open(ncpath.c_str(), NC_NOWRITE, &file.id);
  if (status != NC_NOERR) {
    file.id = -1;
    result.error = std::string("cannot open grid: ") + nc_strerror(status);
    return result;
  }
  const bool recognised = loadLegacy(file.id, result) || loadCoards(file.id, result);
  if (!recognised) {
    result.error = "no gridded variable found (not a GMT grid)";
    return result;
  }
  if (!result.error.empty())
    return result;
  if (!result.grid.valid()) {
    result.error = "grid is empty";
    return result;
  }
  finishGrid(result.grid);
  result.ok = true;
  return result;
}

GridStatistics GridService::statistics(const Grid &grid) const {
  GridStatistics st;
  st.nodes = static_cast<long long>(grid.z.size());
  double sum = 0.0, sum2 = 0.0;
  for (const float z : grid.z) {
    if (std::isfinite(z)) {
      st.valid++;
      sum += z;
      sum2 += static_cast<double>(z) * z;
    }
  }
  st.empty = st.nodes - st.valid;
  st.emptyRatio = st.nodes > 0 ? static_cast<double>(st.empty) / static_cast<double>(st.nodes) : 0.0;
  if (st.valid > 0) {
    st.mean = sum / static_cast<double>(st.valid);
    st.stddev = std::sqrt(std::max(0.0, sum2 / static_cast<double>(st.valid) - st.mean * st.mean));
  }
  return st;
}

}  // namespace mbstudio::mbsystem
