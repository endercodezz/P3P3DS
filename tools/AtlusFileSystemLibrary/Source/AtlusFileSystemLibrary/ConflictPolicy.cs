namespace AtlusFileSystemLibrary
{
    public class ConflictPolicy
    {
        public static ConflictPolicy ThrowError { get; } = new ConflictPolicy( PolicyKind.ThrowError );
        public static ConflictPolicy Replace { get; } = new ConflictPolicy( PolicyKind.Replace );
        public static ConflictPolicy Ignore { get; } = new ConflictPolicy( PolicyKind.Ignore );

        public PolicyKind Kind { get; }

        private ConflictPolicy( PolicyKind policyKind )
        {
            Kind = policyKind;
        }

        public enum PolicyKind
        {
            ThrowError,
            Replace,
            Ignore
        }
    }
}