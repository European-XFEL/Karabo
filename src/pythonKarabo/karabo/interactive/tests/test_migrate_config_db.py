from pathlib import Path

import pytest

from karabo.config_db import ConfigurationDatabase
from karabo.interactive import convert_config_db
from karabo.native import decodeXML


@pytest.mark.asyncio
async def test_migrate_config_db():
    folder = Path(__file__).parent
    dbName = folder.joinpath("karaboDB")
    assert dbName.exists()
    new_db_path = folder.joinpath(convert_config_db.NEW_DB_NAME)
    try:
        await convert_config_db.run(dbName)

        db = ConfigurationDatabase(new_db_path)
        configs = await db.list_devices()
        assert len(configs) == 1

        deviceId = "KaraboConfigurationManager"
        configs = await db.list_configurations(deviceId)
        assert len(configs) == 3

        config2 = configs[1]
        assert config2["name"] == "config2"
        data = await db.get_configuration(deviceId, "config2")
        assert data["deviceId"] == deviceId
        assert data["date"] is not None
        decoded = decodeXML(data["config"])
        assert decoded["priority"] == 2
    finally:
        if new_db_path.exists():
            new_db_path.unlink()
