import pytest
import pytest_asyncio

from ..configuration_database import ConfigurationDatabase, DbHandle
from ..utils import ConfigurationDBError

# Test database URL
TEST_DB_URL = "configtest"


@pytest_asyncio.fixture(loop_scope="module")
async def database():
    db_handle = DbHandle(TEST_DB_URL)
    config_db = ConfigurationDatabase(db_handle)
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
    config1 = {"deviceId": "device_1", "config": "data_1"}
    config2 = {"deviceId": "device_2", "config": "data_2"}
    config3 = {"deviceId": "device_2", "config": "data_3"}
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
    configurations = await database.list_configurations("device_1")
    assert len(configurations) == 1
    assert configurations[0]["name"] == "TestConfig"
    assert configurations[0]["last_loaded"] != ""


@pytest.mark.asyncio
async def test_invalid_configuration(database):
    config = {"deviceId": "device_1"}  # Missing 'config' key
    with pytest.raises(ConfigurationDBError):
        await database.save_configuration("InvalidConfig", [config])
    config = {"config": "data_1"}  # Missing 'deviceId' key
    with pytest.raises(ConfigurationDBError):
        await database.save_configuration("InvalidConfig", [config])
