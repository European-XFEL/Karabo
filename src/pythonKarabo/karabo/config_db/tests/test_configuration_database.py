import pytest
import pytest_asyncio

from ..configuration_database import ConfigurationDatabase
from ..utils import ConfigurationDBError

# Test database URL
TEST_DB_URL = "configtest"


@pytest_asyncio.fixture(loop_scope="module")
async def database():
    config_db = ConfigurationDatabase(TEST_DB_URL)
    await config_db.assure_existing()
    yield config_db
    await config_db.delete()


@pytest.mark.asyncio
async def test_save_and_get_configuration(database):
    config = {
        "deviceId": "device_123",
        "config": "sample_config_data"}

    await database.save_configuration("TestConfig", [config])
    result = await database.get_configuration("device_123", "TestConfig")

    assert result is not None
    assert result["config"] == "sample_config_data"


@pytest.mark.asyncio
async def test_device_config_roundtrip(database):
    # 1. Saving configurations with overwrite
    devices = await database.list_devices()
    assert devices == []
    config1 = {"deviceId": "device_1", "config": "data_1",
               "classId": "testclassId"}
    config2 = {"deviceId": "device_2", "config": "data_2"}
    config3 = {"deviceId": "device_2", "config": "data_3",
               "serverId": "testserver"}
    config4 = {"deviceId": "device_1", "config": "data_4"}
    await database.save_configuration("TestConfig", [config1, config2])
    # Overwrite
    await database.save_configuration("TestConfig", [config1, config3])
    devices = await database.list_devices()
    assert "device_1" in devices
    assert "device_2" in devices

    # 2. Listing Configuration
    configurations = await database.list_configurations("device_1")
    assert len(configurations) == 1
    assert configurations[0]["name"] == "TestConfig"
    assert configurations[0]["timepoint"] is not None
    assert configurations[0]["last_loaded"] == ""

    configurations = await database.list_configurations("device_3")
    assert not len(configurations)
    configurations = await database.list_configurations("device_1", "NO")
    assert not len(configurations)

    # 3. Get a config and check last loaded again
    result = await database.get_configuration("device_1", "TestConfig")
    assert result is not None
    assert result["config"] == "data_1"
    assert result["classId"] == "testclassId"
    result = await database.get_configuration("device_2", "TestConfig")
    assert result is not None
    assert result["config"] == "data_3"
    assert result["serverId"] == "testserver"

    configurations = await database.list_configurations("device_1")
    assert len(configurations) == 1
    assert configurations[0]["name"] == "TestConfig"
    assert configurations[0]["last_loaded"] != ""
    assert configurations[0]["classId"] == "testclassId"
    assert configurations[0]["serverId"] == "__none__"

    # 3. Get a config and check last loaded again, with `new`
    new = {"deviceId": "device_1", "config": "new"}
    await database.save_configuration("TestConfig", [new])
    result = await database.get_configuration("device_1", "TestConfig")
    assert result is not None
    assert result["config"] == "new"

    # 4. try to delete configuration
    # Add one more for device_id
    await database.save_configuration("TestConfig1", [config4])
    with pytest.raises(Exception) as e:
        await database.delete_configuration("device_1", "No")
        assert "not available" in e
    with pytest.raises(Exception) as e:
        await database.delete_configuration("deviceNOT_1", "TestConfig")
        assert "not available" in e
    configurations = await database.list_configurations("device_1")
    assert len(configurations) == 2
    await database.delete_configuration("device_1", "TestConfig")
    configurations = await database.list_configurations("device_1")
    assert len(configurations) == 1
    devices = await database.list_devices()
    assert "device_1" in devices
    assert "device_2" in devices
    await database.delete_configuration("device_1", "TestConfig1")
    # Orphan delete
    configurations = await database.list_configurations("device_1")
    assert len(configurations) == 0
    devices = await database.list_devices()
    assert "device_1" not in devices


@pytest.mark.asyncio
async def test_invalid_configuration(database):
    config = {"deviceId": "device_1"}  # Missing 'config' key
    with pytest.raises(ConfigurationDBError):
        await database.save_configuration("InvalidConfig", [config])
    config = {"config": "data_1"}  # Missing 'deviceId' key
    with pytest.raises(ConfigurationDBError):
        await database.save_configuration("InvalidConfig", [config])
