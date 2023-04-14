using GLib;

public class Player : GLib.Object
{
	public enum State {
		INIT,
		STOPPED,
		PAUSED,
		PLAY_PENDING,
		PLAYING,
	}

	public signal void play();
	public signal void stop();

	public State _state;

	public State state {
		get { return _state; }
		set { _state = value; }
	}

	construct
	{
	}

	public Player()
	{
		print("Player\n");
	}
}

