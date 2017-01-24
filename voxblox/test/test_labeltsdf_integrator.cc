#include <gtest/gtest.h>

#include "voxblox/core/labeltsdf_map.h"
#include "voxblox/integrator/labeltsdf_integrator.h"
#include "voxblox/io/layer_io.h"
#include "voxblox/io/mesh_ply.h"
#include "voxblox/mesh/mesh_label_integrator.h"
#include "voxblox/test/layer_test_utils.h"

namespace voxblox {

class LabelTsdfIntegratorTest : public testing::Test {
 protected:
  virtual void SetUp() {
    LabelTsdfMap::Config map_config;
    map_config.voxel_size = 0.1f;
    map_config.voxels_per_side = 8u;
    map_.reset(new LabelTsdfMap(map_config));

    LabelTsdfIntegrator::Config integrator_config;
    integrator_.reset(new LabelTsdfIntegrator(integrator_config,
                                              map_->getTsdfLayerPtr(),
                                              map_->getLabelLayerPtr(),
                                              map_->getHighestLabelPtr()));
   }

   std::shared_ptr<LabelTsdfMap> map_;
   std::shared_ptr<LabelTsdfIntegrator> integrator_;

   voxblox::test::LabelLayerTest label_layer_test_;
   voxblox::test::TsdfLayerTest tsdf_layer_test_;
};

TEST_F(LabelTsdfIntegratorTest, IntegratePointCloud) {
  Point sensor_origin(0, 0, 0);
  Transformation transform(sensor_origin, Eigen::Quaterniond::Identity());

  Pointcloud frame_to_integrate;

  Colors colors_to_integrate;
  Labels labels_to_integrate;

  size_t label = 1u;
  // Build a 2x2m regular grid pointcloud.
  for (float x = 0.0f; x < 2.0f; x = x + 0.05f) {
    for (float z = 0.0f; z < 2.0f; z = z + 0.05f) {
      frame_to_integrate.push_back(
          transform.inverse() * Point(x, 1, z));
      // Assign a dummy color to the pointcloud to integrate.
      colors_to_integrate.push_back(Color());
      // Assign the same label to the whole pointcloud to integrate.
      labels_to_integrate.push_back(label);
    }
  }

  integrator_->integratePointCloud(transform,
                                   frame_to_integrate,
                                   colors_to_integrate,
                                   labels_to_integrate);

  // Read tsdf and label layers ground truth from file
  const std::string tsdf_file = "labeltsdf_integrator_test_1.tsdf.voxblox";
  Layer<TsdfVoxel>::Ptr tsdf_layer_from_file;
  io::LoadLayer<TsdfVoxel>(tsdf_file, &tsdf_layer_from_file);

  tsdf_layer_test_.CompareLayers(map_->getTsdfLayer(), *tsdf_layer_from_file);

  const std::string label_file = "labeltsdf_integrator_test_1.label.voxblox";
  Layer<LabelVoxel>::Ptr label_layer_from_file;
  io::LoadLayer<LabelVoxel>(label_file, &label_layer_from_file);

  label_layer_test_.CompareLayers(map_->getLabelLayer(), *label_layer_from_file);
}

TEST_F(LabelTsdfIntegratorTest, ReadLabelPointCloud) {
  Point sensor_origin(0, 0, 0);
  Transformation transform(sensor_origin, Eigen::Quaterniond::Identity());

  Pointcloud frame_to_integrate;
  Pointcloud frame_to_compute_labels;

  Colors colors_to_integrate;
  Labels labels_to_integrate;

  size_t label = 1u;
  // Build two 2x2m regular grid pointclouds.
  for (float x = 0.0f; x < 2.0f; x = x + 0.05f) {
    for (float z = 0.0f; z < 2.0f; z = z + 0.05f) {
      frame_to_integrate.push_back(transform.inverse() * Point(x, 1, z));
      frame_to_compute_labels.push_back(transform.inverse() * Point(x, 1, z));

      // Assign a dummy color to the pointcloud to integrate.
      colors_to_integrate.push_back(Color());
      // Assign the same label to the whole pointcloud to integrate.
      labels_to_integrate.push_back(label);
    }
  }

  integrator_->integratePointCloud(transform,
                                   frame_to_integrate,
                                   colors_to_integrate,
                                   labels_to_integrate);


  Labels computed_labels;
  integrator_->computePointCloudLabel(transform,
                                      frame_to_compute_labels,
                                      &computed_labels);

  // The computed labels match exactly the ones integrated
  EXPECT_TRUE(std::equal(computed_labels.begin(), computed_labels.end(),
                         labels_to_integrate.begin()));
}

TEST_F(LabelTsdfIntegratorTest, ComputeDominantLabelPointCloud) {
  Point sensor_origin(0, 0, 0);
  Transformation transform(sensor_origin, Eigen::Quaterniond::Identity());

  Pointcloud frame_to_integrate;
  Pointcloud frame_to_compute_labels;

  Colors colors_to_integrate;
  Labels labels_to_integrate;

  size_t label = 1u;
  // Build two 2x2m regular grid pointclouds.
  for (float x = 0.0f; x < 2.0f; x = x + 0.05f) {
    for (float z = 0.0f; z < 2.0f; z = z + 0.05f) {
      frame_to_integrate.push_back(transform.inverse() * Point(x, 1, z));
      frame_to_compute_labels.push_back(transform.inverse() * Point(x, 1, z));

      // Assign a dummy color to the pointcloud to integrate.
      colors_to_integrate.push_back(Color());
      // Assign two different labels to different.
      // parts of the pointcloud to integrate.
      if (x > 1.5f) {
        label = 2u;
      }
      labels_to_integrate.push_back(label);
    }
  }

  integrator_->integratePointCloud(transform,
                                   frame_to_integrate,
                                   colors_to_integrate,
                                   labels_to_integrate);

  Labels computed_labels;
  integrator_->computePointCloudLabel(transform,
                                      frame_to_compute_labels,
                                      &computed_labels);
  // The computed labels are all 1 since it's the dominant integrated label
  Labels expected_labels(computed_labels.size(), 1);

  EXPECT_TRUE(std::equal(computed_labels.begin(), computed_labels.end(),
                         expected_labels.begin()));

  // TODO(grinvalm) trigger test results visualization based on a flag
  // Generate the mesh.
  MeshLayer mesh_layer(map_->block_size());
  MeshLabelIntegrator::Config mesh_config;
  MeshLabelIntegrator mesh_integrator(mesh_config,
                                      map_->getTsdfLayerPtr(),
                                      map_->getLabelLayerPtr(),
                                      &mesh_layer);

  mesh_integrator.generateWholeMesh();

  voxblox::outputMeshLayerAsPly("test_tsdf.ply", mesh_layer);
}

TEST_F(LabelTsdfIntegratorTest, ComputeUnseenLabelPointCloud) {
  Point sensor_origin(0, 0, 0);
  Transformation transform(sensor_origin, Eigen::Quaterniond::Identity());

  Pointcloud frame_to_integrate;
  Pointcloud frame_to_compute_labels;

  Colors colors_to_integrate;
  Labels labels_to_integrate;

  size_t label = 1u;
  // Build two 2x2m regular grid pointclouds.
  for (float x = 0.0f; x < 2.0f; x = x + 0.05f) {
    for (float z = 0.0f; z < 2.0f; z = z + 0.05f) {
      frame_to_integrate.push_back(
          transform.inverse() * Point(x, 1, z));
      // Read labels for a pointcloud for an unobserved area
      frame_to_compute_labels.push_back(
          transform.inverse() * Point(x + 2.5, 1, z));

      // Assign a dummy color to the pointcloud to integrate.
      colors_to_integrate.push_back(Color());
      // Assign the same label to the whole pointcloud to integrate.
      labels_to_integrate.push_back(label);
    }
  }

  integrator_->integratePointCloud(transform,
                                   frame_to_integrate,
                                   colors_to_integrate,
                                   labels_to_integrate);

  Labels computed_labels;
  integrator_->computePointCloudLabel(transform,
                                      frame_to_compute_labels,
                                      &computed_labels);

  // The computed labels are all the unseen label 2.
  Labels expected_labels(computed_labels.size(), 2);

  EXPECT_TRUE(std::equal(computed_labels.begin(), computed_labels.end(),
                         expected_labels.begin()));
}
}  // namespace voxblox

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);

  int result = RUN_ALL_TESTS();

  return result;
}
